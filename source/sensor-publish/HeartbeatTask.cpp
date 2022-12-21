// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "HeartbeatTask.h"

#include "../Feature.h"
#include "../logging/LoggerFactory.h"

#include <aws/common/byte_buf.h>
#include <aws/common/error.h>
#include <aws/common/task_scheduler.h>
#include <aws/common/zero.h>
#include <aws/crt/mqtt/MqttClient.h>
#include <aws/io/event_loop.h>
#include <aws/mqtt/client.h>

#include <chrono>
#include <cstdint>

using namespace std;
using namespace Aws::Iot;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::Logging;
using namespace Aws::Iot::DeviceClient::SensorPublish;

constexpr char HeartbeatTask::TAG[];

HeartbeatTask::HeartbeatTask(
    const SensorState &state,
    const PlainConfig::SensorPublish::SensorSettings &settings,
    shared_ptr<Crt::Mqtt::MqttConnection> connection,
    aws_event_loop *eventLoop)
    : mState(state), mSettings(settings), mConnection(connection), mEventLoop(eventLoop)
{
    // Initialize a task to publish heartbeat to MQTT from the event loop.
    // Only needs to be done once.
    AWS_ZERO_STRUCT(mTask);
    aws_task_init(
        &mTask,
        [](struct aws_task *, void *arg, enum aws_task_status status) {
            if (status == AWS_TASK_STATUS_CANCELED)
            {
                return; // Ignore canceled tasks.
            }
            auto *self = static_cast<HeartbeatTask *>(arg);
            self->publishHeartbeat();
        },
        this,
        __func__);

    // Since heartbeat topic and payload never change, initialize a cursor with statically allocated memory.
    if (enabled())
    {
        mTopic = aws_byte_cursor_from_c_str(mSettings.mqttHeartbeatTopic->c_str());
    }
    else
    {
        // Heartbeat is not enabled, zero struct to prevent accidental use.
        AWS_ZERO_STRUCT(mTopic);
    }
    mPayload = aws_byte_cursor_from_c_str(mSettings.name->c_str());
}

bool HeartbeatTask::enabled() const
{
    return mSettings.mqttHeartbeatTopic.has_value() && !mSettings.mqttHeartbeatTopic.value().empty();
}

int HeartbeatTask::start()
{
    // Check for previously running task and stop.
    if (mStarted)
    {
        stop();
    }

    // Unspecified topic means heartbeat is not enabled.
    if (enabled())
    {
        scheduleHeartbeat();
        mStarted = true;
    }

    return Feature::SUCCESS;
}

int HeartbeatTask::stop()
{
    if (mStarted)
    {
        // Cancel the current task.
        if (aws_event_loop_thread_is_callers_thread(mEventLoop))
        {
            aws_event_loop_cancel_task(mEventLoop, &mTask);
        }
        mStarted = false;
    }

    return Feature::SUCCESS;
}

void HeartbeatTask::publishHeartbeat()
{
    // Sensor is connected, but heartbeat has been stopped.
    if (!mStarted)
    {
        return;
    }

    // No heartbeat published when sensor is not connected.
    if (mState < SensorState::Connected)
    {
        // Schedule the next heartbeat check.
        scheduleHeartbeat();
        return;
    }

    // Publish the heartbeat message.
    publish();
}

void HeartbeatTask::publish()
{
    aws_mqtt_client_connection_publish(
        mConnection->GetUnderlyingConnection(),
        &mTopic,
        AWS_MQTT_QOS_AT_LEAST_ONCE,
        false,
        &mPayload,
        [](struct aws_mqtt_client_connection *, uint16_t packet_id, int error_code, void *userdata) {
            auto *self = static_cast<HeartbeatTask *>(userdata);
            if (error_code)
            {
                LOGM_ERROR(
                    TAG,
                    "Error heartbeat sensor name: %s func: %s msg: %s",
                    self->mSettings.name->c_str(),
                    __func__,
                    aws_error_str(error_code));
            }
            else
            {
                LOGM_DEBUG(
                    TAG, "Publish heartbeat sensor name: %s packetId: %d", self->mSettings.name->c_str(), packet_id);
            }
            // Schedule the next heartbeat check.
            if (self->mStarted)
            {
                self->scheduleHeartbeat();
            }
        },
        this);
}

void HeartbeatTask::scheduleHeartbeat()
{
    uint64_t runAtNanos;
    aws_event_loop_current_clock_time(mEventLoop, &runAtNanos);
    chrono::seconds delaySec(mSettings.heartbeatTimeSec.value());
    runAtNanos += chrono::duration_cast<chrono::nanoseconds>(delaySec).count();
    aws_event_loop_schedule_task_future(mEventLoop, &mTask, runAtNanos);
}
