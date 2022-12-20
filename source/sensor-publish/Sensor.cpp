// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "Sensor.h"

#include "../Feature.h"
#include "../logging/LoggerFactory.h"

#include <aws/common/allocator.h>
#include <aws/common/byte_buf.h>
#include <aws/common/error.h>
#include <aws/common/task_scheduler.h>
#include <aws/common/zero.h>
#include <aws/crt/mqtt/MqttClient.h>
#include <aws/io/event_loop.h>
#include <aws/io/socket.h>
#include <aws/mqtt/client.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>

using namespace std;
using namespace Aws::Iot;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::Logging;
using namespace Aws::Iot::DeviceClient::SensorPublish;
using namespace Aws::Crt::Mqtt;

constexpr char Sensor::TAG[];

Sensor::Sensor(
    const PlainConfig::SensorPublish::SensorSettings &settings,
    aws_allocator *allocator,
    shared_ptr<Crt::Mqtt::MqttConnection> connection,
    aws_event_loop *eventLoop,
    shared_ptr<Socket> socket)
    : mSettings(settings), mAllocator(allocator), mConnection(connection), mEventLoop(eventLoop), mSocket(socket),
      mEomPattern(settings.eomDelimiter.value()), mHeartbeatTask(mState, mSettings, mConnection, mEventLoop)
{
    // Handle out of memory when allocating read buffer.
    AWS_ZERO_STRUCT(mReadBuf);
    if (aws_byte_buf_init(&mReadBuf, mAllocator, mSettings.bufferCapacity.value()) != AWS_OP_SUCCESS)
    {
        throw std::runtime_error{"Unable to allocate memory for read buffer"};
    }

    // Since topic never changes, initialize a cursor with statically allocated memory.
    mTopic = aws_byte_cursor_from_c_str(mSettings.mqttTopic->c_str());

    // Initialize a task to connect to sensor socket from the event loop.
    // Only needs to be done once.
    AWS_ZERO_STRUCT(mConnectTask);
    aws_task_init(
        &mConnectTask,
        [](struct aws_task *, void *arg, enum aws_task_status status) {
            if (status == AWS_TASK_STATUS_CANCELED)
            {
                return; // Ignore canceled tasks.
            }
            auto *self = static_cast<Sensor *>(arg);
            self->onConnectTaskCallback();
        },
        this,
        __func__);
}

Sensor::~Sensor()
{
    if (mSocket->is_open())
    {
        mState = SensorState::NotConnected;
        mSocket->close();
    }
    mSocket->clean_up();
    mHeartbeatTask.stop();
    aws_byte_buf_clean_up_secure(&mReadBuf);
}

int Sensor::start()
{
    LOGM_DEBUG(TAG, "Starting sensor name: %s", mSettings.name->c_str());
    connect();
    mHeartbeatTask.start();
    return Feature::SUCCESS;
}

int Sensor::stop()
{
    LOGM_DEBUG(TAG, "Stopping sensor name: %s", mSettings.name->c_str());
    close();
    reset();
    mHeartbeatTask.stop();
    return Feature::SUCCESS;
}

string Sensor::getName() const
{
    return mSettings.name.value();
}

void Sensor::connect(bool delay)
{
    if (mState != SensorState::NotConnected)
    {
        return; // Ignore already connected socket.
    }

    // Cancel pending tasks, if any.
    if (mState == SensorState::Connecting)
    {
        if (aws_event_loop_thread_is_callers_thread(mEventLoop))
        {
            aws_event_loop_cancel_task(mEventLoop, &mConnectTask);
        }
    }

    mState = SensorState::Connecting;

    // Schedule task to cnnect to sensor socket.
    if (delay && mSettings.addrPollSec.value() > 0)
    {
        // Schedule task in the future.
        uint64_t runAtNanos;
        aws_event_loop_current_clock_time(mEventLoop, &runAtNanos);
        chrono::seconds delaySec(mSettings.addrPollSec.value());
        runAtNanos += chrono::duration_cast<chrono::nanoseconds>(delaySec).count();
        aws_event_loop_schedule_task_future(mEventLoop, &mConnectTask, runAtNanos);
    }
    else
    {
        // Schedule task immediately.
        aws_event_loop_schedule_task_now(mEventLoop, &mConnectTask);
    }
}

void Sensor::onConnectTaskCallback()
{
    aws_socket_options socket_options;
    socket_options.type = AWS_SOCKET_STREAM;
    socket_options.domain = AWS_SOCKET_LOCAL;
    mSocket->init(mAllocator, &socket_options);

    aws_socket_endpoint endpoint{};
    AWS_ZERO_STRUCT(endpoint);
    snprintf(endpoint.address, AWS_ADDRESS_MAX_LEN, "%s", mSettings.addr->c_str());

    int rc = mSocket->connect(
        &endpoint,
        mEventLoop,
        [](struct aws_socket *, int error_code, void *user_data) {
            auto *self = static_cast<Sensor *>(user_data);
            self->onConnectionResultCallback(error_code);
        },
        this);
    if (rc != AWS_OP_SUCCESS)
    {
        // Log an error, clean up socket, and reconnect to sensor.
        LOGM_ERROR(
            TAG,
            "Error sensor name: %s func: aws_socket_connect msg: %s",
            mSettings.name->c_str(),
            aws_error_str(aws_last_error()));
        mSocket->clean_up();
        mState = SensorState::NotConnected;
        connect(true);
    }
    else
    {
        LOGM_DEBUG(TAG, "Success sensor name: %s func: aws_socket_connect", mSettings.name->c_str());
    }
}

void Sensor::onConnectionResultCallback(int error_code)
{
    if (error_code)
    {
        // Log an error, close socket, and reconnect to sensor.
        LOGM_ERROR(
            TAG,
            "Error sensor name: %s func: %s msg: %s",
            mSettings.name->c_str(),
            __func__,
            aws_error_str(error_code));
        close();
        connect(true);
    }
    else
    {
        mState = SensorState::Connected;
        LOGM_DEBUG(TAG, "Success sensor name: %s func: %s", mSettings.name->c_str(), __func__);

        // Publish any previously buffered data.
        publish();

        // Update the publish timeout.
        if (mSettings.bufferTimeMs.value() > 0)
        {
            chrono::milliseconds delayMs(mSettings.bufferTimeMs.value());
            mNextPublishTimeout = chrono::high_resolution_clock::now() + delayMs;
        }

        // Register callback that will be invoked when the socket is readable.
        mSocket->subscribe_to_readable_events(
            [](struct aws_socket *, int error_code, void *user_data) {
                auto *self = static_cast<Sensor *>(user_data);
                self->onReadableCallback(error_code);
            },
            this);
    }
}

void Sensor::onReadableCallback(int error_code)
{
    if (error_code)
    {
        // Log an error, close socket, and reconnect to sensor.
        LOGM_ERROR(
            TAG,
            "Error sensor name: %s func: %s msg: %s",
            mSettings.name->c_str(),
            __func__,
            aws_error_str(error_code));
        close();
        connect(true);
    }
    else
    {
        // The event loop used for reading sensor data is edge triggered.
        //
        // In order to avoid stalling the event loop by leaving unread
        // bytes in the socket, the loop below will read from the socket
        // until receiving AWS_IO_READ_WOULD_BLOCK.
        //
        // This behavior is equivalent to using epoll on Linux with socket
        // added as EPOLLET (edge-triggered) and reading until receiving EAGAIN.
        bool readWouldBlock = false;
        while (!readWouldBlock)
        {
            size_t numRead = 0, startPos = mReadBuf.len;
            int rc = mSocket->read(&mReadBuf, &numRead);
            if (rc == AWS_OP_SUCCESS)
            {
                LOGM_DEBUG(TAG, "Read sensor name: %s bytes: %zu", mSettings.name->c_str(), numRead);

                // Scan the buffer for end of message boundaries.
                // If the buffer is empty, then start scan from start of read.
                // If the buffer is not empty, then start from one past end of last message.
                const char *pbuf = reinterpret_cast<char *>(mReadBuf.buffer);
                auto beginPos = mEomBounds.empty() ? startPos : mEomBounds.back();
                for (auto m = cregex_iterator(pbuf + beginPos, pbuf + startPos + numRead, mEomPattern),
                          mend = cregex_iterator();
                     m != mend;
                     ++m)
                {
                    // Store the position of one-past the end of the match.
                    mEomBounds.emplace(beginPos + (*m).position() + (*m).length());
                }

                // Invoke publish to check whether batch limits are breached.
                publish();
            }
            else
            {
                int lastError = aws_last_error();
                if (lastError == AWS_IO_READ_WOULD_BLOCK)
                {
                    // Wait for socket to become readable again before trying to read.
                    readWouldBlock = true;
                }
                else
                {
                    if (lastError == AWS_IO_SOCKET_NOT_CONNECTED || lastError == AWS_IO_SOCKET_CLOSED)
                    {
                        // Close socket, and reconnect to sensor.
                        close();
                        connect(true);
                    }
                    else
                    {
                        // Log an error and wait for socket to become readable before trying to read.
                        LOGM_ERROR(
                            TAG,
                            "Error sensor name: %s func: aws_socket_read msg: %s",
                            mSettings.name->c_str(),
                            aws_error_str(lastError));
                    }
                    return;
                }
            }
        }
    }
}

void Sensor::publish()
{
    // Check whether limits are breached and, if so, compute bufferSize and numBatches.
    size_t bufferSize, numBatches;
    if (!needPublish(bufferSize, numBatches))
    {
        LOGM_DEBUG(TAG, "Nothing to publish sensor name: %s", mSettings.name->c_str());
        return;
    }

    size_t startPos = 0, lastEom = 0;
    while (numBatches > 0)
    {
        // Publish complete messages in bufferSize increments.
        size_t numToPub = min(mEomBounds.size(), bufferSize);
        for (size_t i = 0; i < numToPub; ++i)
        {
            lastEom = mEomBounds.front();
            mEomBounds.pop();
        }

        // Create a shallow copy of a buffer up to the lastEom.
        aws_byte_cursor pubBuf = aws_byte_cursor_from_array(mReadBuf.buffer + startPos, lastEom);
        LOGM_DEBUG(TAG, "Publish sensor name: %s bytes: %zu", mSettings.name->c_str(), pubBuf.len);

        // Publish buffer.
        publishOneMessage(&pubBuf);

        // Update start of next message to 1-past end of current message.
        startPos = lastEom;

        --numBatches;
    }

    if (mEomBounds.empty())
    {
        // Advance the lastEom to the start of the buffer.
        size_t count = mReadBuf.len - lastEom;
        std::memcpy(mReadBuf.buffer, mReadBuf.buffer + lastEom, count);
        mReadBuf.len = count;
    }

    // Update the publish timeout.
    if (mSettings.bufferTimeMs.value() > 0)
    {
        chrono::milliseconds delayMs(mSettings.bufferTimeMs.value());
        mNextPublishTimeout = chrono::high_resolution_clock::now() + delayMs;
    }
}

bool Sensor::needPublish(size_t &bufferSize, size_t &numBatches)
{
    // Buffer size is the number of messages published in a single batch.
    bufferSize = size_t(mSettings.bufferSize.value());
    if (bufferSize == 0)
    {
        // Publish all buffered messages as a single batch.
        bufferSize = mEomBounds.size();
    }

    // Compute number of batches based on the total available messages.
    numBatches = mEomBounds.empty() ? 0 : mEomBounds.size() / bufferSize;

    if (numBatches < 1)
    {
        if (!mEomBounds.empty())
        {
            // Check other circumstances in which we must publish at least one batch.
            if (chrono::high_resolution_clock::now() > mNextPublishTimeout)
            {
                numBatches = 1; // Publish timeout.
            }
            else if (mReadBuf.len == mReadBuf.capacity)
            {
                numBatches = 1; // Buffer full.
            }
        }
        else
        {
            // Discard unpublished data when buffer is full and we haven't
            // found any end of message delimeters in the buffer.
            if (mReadBuf.len == mReadBuf.capacity)
            {
                LOGM_ERROR(
                    TAG,
                    "Buffer is full and no end of message delimeter detected, discarding %zu bytes of unpublished "
                    "messages sensor name: %s",
                    mReadBuf.len,
                    mSettings.name->c_str());
                aws_byte_buf_reset(&mReadBuf, false);
            }
        }
    }

    return numBatches > 0;
}

void Sensor::publishOneMessage(const aws_byte_cursor *payload)
{
    aws_mqtt_client_connection_publish(
        mConnection->GetUnderlyingConnection(),
        &mTopic,
        AWS_MQTT_QOS_AT_LEAST_ONCE,
        false,
        payload,
        [](struct aws_mqtt_client_connection *, uint16_t packet_id, int error_code, void *userdata) {
            auto *self = static_cast<Sensor *>(userdata);
            if (error_code)
            {
                // Log an error, but otherwise discard the message data.
                LOGM_ERROR(
                    TAG,
                    "Error sensor name: %s func: %s msg: %s",
                    self->mSettings.name->c_str(),
                    __func__,
                    aws_error_str(error_code));
            }
            else
            {
                LOGM_DEBUG(
                    TAG, "Publish complete sensor name: %s packetId: %d", self->mSettings.name->c_str(), packet_id);
            }
        },
        this);
}

void Sensor::close()
{
    if (mSocket->is_open())
    {
        mState = SensorState::NotConnected;

        if (mSocket->close() != AWS_OP_SUCCESS)
        {
            LOGM_ERROR(
                TAG,
                "Error sensor name: %s func: aws_socket_close msg: %s",
                mSettings.name->c_str(),
                aws_error_str(aws_last_error()));
        }
        else
        {
            LOGM_DEBUG(TAG, "Sensor socket closed name: %s", mSettings.name->c_str());
        }
    }
}

void Sensor::reset()
{
    aws_byte_buf_reset(&mReadBuf, false);
    while (!mEomBounds.empty())
    {
        mEomBounds.pop();
    }
}
