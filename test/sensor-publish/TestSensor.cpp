// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/sensor-publish/Sensor.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <aws/common/allocator.h>
#include <aws/common/clock.h>
#include <aws/common/error.h>
#include <aws/crt/Types.h>
#include <aws/crt/mqtt/MqttClient.h>
#include <aws/io/event_loop.h>

#include <chrono>
#include <memory>
#include <string>
#include <vector>

using namespace Aws::Iot;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::SensorPublish;

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::NiceMock;

class SensorTest : public ::testing::Test
{
  public:
    void SetUp() override
    {
        // Configure settings used by Sensor.
        settings.name = "my-sensor";
        settings.addr = "my-sensor-server";
        settings.mqttTopic = "my-sensor-data";
        settings.eomDelimiter = "[,]+";
        settings.bufferCapacity = 1024;
        settings.addrPollSec = 0; // No delay between reconnect.

        // Initialize default allocator.
        allocator = aws_default_allocator();

        // Initialize and start the event loop.
        eventLoop = aws_event_loop_new_default(allocator, aws_high_res_clock_get_ticks);
    }

    void TearDown() override { aws_event_loop_destroy(eventLoop); }

    PlainConfig::SensorPublish::SensorSettings settings;
    aws_allocator *allocator;
    std::shared_ptr<Aws::Crt::Mqtt::MqttConnection> connection;
    aws_event_loop *eventLoop;
};

class MockSensor : public Sensor
{
  public:
    MockSensor(
        const PlainConfig::SensorPublish::SensorSettings &settings,
        aws_allocator *allocator,
        std::shared_ptr<Aws::Crt::Mqtt::MqttConnection> connection,
        aws_event_loop *eventLoop,
        std::shared_ptr<Socket> socket)
        : Sensor(settings, allocator, connection, eventLoop, socket)
    {
    }

    void call_onConnectTaskCallback() { onConnectTaskCallback(); }

    void call_onConnectionResultCallback(int error_code) { onConnectionResultCallback(error_code); }

    void call_onReadableCallback(int error_code) { onReadableCallback(error_code); }

    bool needPublish(size_t &bufferSize, size_t &numBatches) { return Sensor::needPublish(bufferSize, numBatches); }

    SensorState getState() const { return mState; }

    void nextPublishTimeout(int64_t delay_ms)
    {
        mNextPublishTimeout = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds{delay_ms};
    }

    void addMessages(size_t count)
    {
        for (size_t i = 1; i <= count; ++i)
        {
            mEomBounds.push(i);
        }
    }

    void writeReadBuf(size_t count) { mReadBuf.len += count; }

    size_t getReadBufLen() const { return mReadBuf.len; }

    size_t getEomBoundsSize() const { return mEomBounds.size(); }

    std::vector<size_t> getEomBounds() const
    {
        auto copyEomBounds(mEomBounds);
        std::vector<size_t> bounds;
        while (!copyEomBounds.empty())
        {
            bounds.push_back(copyEomBounds.front());
            copyEomBounds.pop();
        }
        return bounds;
    }

    MOCK_METHOD(void, connect, (bool delay), (override));
    MOCK_METHOD(void, publish, (), (override));
    MOCK_METHOD(void, close, (), (override));
};

class FakeSocket : public Socket
{
  public:
    void init(aws_allocator *allocator, aws_socket_options *options) override {}

    int connect(
        const struct aws_socket_endpoint *remote_endpoint,
        struct aws_event_loop *event_loop,
        aws_socket_on_connection_result_fn *on_connection_result,
        void *user_data) override
    {
        return AWS_OP_SUCCESS;
    }

    int subscribe_to_readable_events(aws_socket_on_readable_fn *on_readable, void *user_data) override
    {
        return AWS_OP_SUCCESS;
    }

    bool is_open() override { return true; }

    int read(aws_byte_buf *buf, std::size_t *amount_read) override { return AWS_OP_SUCCESS; }

    int close() override { return AWS_OP_SUCCESS; }

    void clean_up() override {}
};

TEST_F(SensorTest, SensorSocketConnectSuccess)
{
    // When connect task callback is invoked and socket connect is successful,
    // then no attempt to reconnect to sensor.
    auto socket = std::make_shared<FakeSocket>();
    MockSensor sensor(settings, allocator, connection, eventLoop, socket);
    EXPECT_CALL(sensor, connect(true)).Times(0); // No reconnect.

    sensor.call_onConnectTaskCallback();
}

class FakeSocketConnectFails : public FakeSocket
{
  public:
    int connect(
        const struct aws_socket_endpoint *remote_endpoint,
        struct aws_event_loop *event_loop,
        aws_socket_on_connection_result_fn *on_connection_result,
        void *user_data) override
    {
        return aws_raise_error(AWS_IO_SOCKET_CONNECTION_REFUSED);
    }
};

TEST_F(SensorTest, SensorSocketConnectFails)
{
    // When connect task callback is invoked and socket connect fails,
    // then attempt to reconnect to sensor.
    auto socket = std::make_shared<FakeSocketConnectFails>();
    MockSensor sensor(settings, allocator, connection, eventLoop, socket);
    EXPECT_CALL(sensor, connect(true)).Times(1); // Reconnect.

    sensor.call_onConnectTaskCallback();
}

class FakeSocketCountSubscribeEvents : public FakeSocket
{
  public:
    int subscribe_to_readable_events(aws_socket_on_readable_fn *on_readable, void *user_data) override
    {
        ++count;
        return AWS_OP_SUCCESS;
    }
    int count{0};
};

TEST_F(SensorTest, SensorSocketConnectionResultSuccess)
{
    // When connect result callback returns success,
    // then subscribe to readable events.
    auto socket = std::make_shared<FakeSocketCountSubscribeEvents>();
    NiceMock<MockSensor> sensor(settings, allocator, connection, eventLoop, socket);
    EXPECT_CALL(sensor, close()).Times(0);
    EXPECT_CALL(sensor, connect(true)).Times(0); // No reconnect.

    sensor.call_onConnectionResultCallback(AWS_OP_SUCCESS);
    ASSERT_EQ(socket->count, 1);
}

TEST_F(SensorTest, SensorSocketConnectionResultFails)
{
    // When connect result callback returns success,
    // then do not subscribe to readable events and close and reconnect socket.
    auto socket = std::make_shared<FakeSocketCountSubscribeEvents>();
    MockSensor sensor(settings, allocator, connection, eventLoop, socket);
    EXPECT_CALL(sensor, close()).Times(1);
    EXPECT_CALL(sensor, connect(true)).Times(1); // Reconnect.

    sensor.call_onConnectionResultCallback(AWS_OP_ERR);
    ASSERT_EQ(socket->count, 0);
}

TEST_F(SensorTest, SocketOnReadableFails)
{
    // When socket readable callback returns error,
    // then readable callback will exit without publishing as well as close and reconnect the socket.
    auto socket = std::make_shared<FakeSocket>();
    MockSensor sensor(settings, allocator, connection, eventLoop, socket);
    EXPECT_CALL(sensor, publish()).Times(0);
    EXPECT_CALL(sensor, close()).Times(1);
    EXPECT_CALL(sensor, connect(true)).Times(1); // Reconnect.

    sensor.call_onReadableCallback(AWS_IO_SOCKET_NOT_CONNECTED);
}

class FakeSocketReadNTimes : public FakeSocket
{
  public:
    FakeSocketReadNTimes(int ntimes) : ntimes(ntimes) {}
    int read(aws_byte_buf *buf, std::size_t *amount_read) override
    {
        if (ntimes-- > 0)
        {
            return AWS_OP_SUCCESS;
        }
        return aws_raise_error(AWS_IO_READ_WOULD_BLOCK);
    }
    int ntimes{0};
};

TEST_F(SensorTest, SocketReadSuccess)
{
    // When socket readable callback returns success and socket read returns success,
    // then readable callback will exit after publishing without closing socket.
    int nreads{2};
    auto socket = std::make_shared<FakeSocketReadNTimes>(nreads);
    MockSensor sensor(settings, allocator, connection, eventLoop, socket);
    EXPECT_CALL(sensor, publish()).Times(nreads);
    EXPECT_CALL(sensor, close()).Times(0);
    EXPECT_CALL(sensor, connect(true)).Times(0); // No reconnect.

    sensor.call_onReadableCallback(AWS_OP_SUCCESS);
}

class FakeSocketReadWouldBlock : public FakeSocket
{
  public:
    int read(aws_byte_buf *buf, std::size_t *amount_read) override { return aws_raise_error(AWS_IO_READ_WOULD_BLOCK); }
};

TEST_F(SensorTest, SocketReadWouldBlock)
{
    // When socket is readable and read returns AWS_IO_READ_WOULD_BLOCK error,
    // then readable callback will exit without publishing, closing, or reconnecting socket.
    auto socket = std::make_shared<FakeSocketReadWouldBlock>();
    MockSensor sensor(settings, allocator, connection, eventLoop, socket);
    EXPECT_CALL(sensor, publish()).Times(0);
    EXPECT_CALL(sensor, close()).Times(0);
    EXPECT_CALL(sensor, connect(_)).Times(0);

    sensor.call_onReadableCallback(AWS_OP_SUCCESS);
}

class FakeSocketReadFails : public FakeSocket
{
  public:
    int read(aws_byte_buf *buf, std::size_t *amount_read) override { return aws_raise_error(AWS_IO_SOCKET_CLOSED); }
};

TEST_F(SensorTest, SocketReadFails)
{
    // When socket is readable and read fails,
    // then readable callback will exit without publishing as well as close and reconnect the socket.
    auto socket = std::make_shared<FakeSocketReadFails>();
    MockSensor sensor(settings, allocator, connection, eventLoop, socket);
    EXPECT_CALL(sensor, publish()).Times(0);
    EXPECT_CALL(sensor, close()).Times(1);
    EXPECT_CALL(sensor, connect(true)).Times(1); // Reconnect.

    sensor.call_onReadableCallback(AWS_OP_SUCCESS);
}

class FakeSocketReadData : public FakeSocket
{
  public:
    int read(aws_byte_buf *buf, std::size_t *amount_read) override
    {
        if (count < dataToWrite.size())
        {
            const uint8_t *src = reinterpret_cast<const uint8_t *>(dataToWrite[count].data());
            aws_byte_buf_write(buf, src, dataToWrite[count].size());
            *amount_read = dataToWrite[count].size();
            ++count;
            return AWS_OP_SUCCESS;
        }
        return aws_raise_error(AWS_IO_READ_WOULD_BLOCK);
    }
    std::vector<std::string> dataToWrite;
    size_t count{0};
};

TEST_F(SensorTest, ScanEomNoMatch)
{
    // When data contains no eom, then no eom match found.
    auto socket = std::make_shared<FakeSocketReadData>();
    socket->dataToWrite.emplace_back("data with no eom");
    MockSensor sensor(settings, allocator, connection, eventLoop, socket);
    EXPECT_CALL(sensor, publish()).Times(1);
    EXPECT_CALL(sensor, close()).Times(0);
    EXPECT_CALL(sensor, connect(_)).Times(0);

    sensor.call_onReadableCallback(AWS_OP_SUCCESS);
    ASSERT_EQ(sensor.getEomBoundsSize(), 0); // No EOM match.
}

TEST_F(SensorTest, ScanEomOneMatch)
{
    // When data contains 1 eom, then 1 eom match found.
    auto socket = std::make_shared<FakeSocketReadData>();
    socket->dataToWrite.emplace_back("msg1,msg2");
    MockSensor sensor(settings, allocator, connection, eventLoop, socket);
    EXPECT_CALL(sensor, publish()).Times(1);
    EXPECT_CALL(sensor, close()).Times(0);
    EXPECT_CALL(sensor, connect(_)).Times(0);

    sensor.call_onReadableCallback(AWS_OP_SUCCESS);
    ASSERT_EQ(sensor.getEomBoundsSize(), 1); // 1 EOM match.
    ASSERT_THAT(sensor.getEomBounds(), ElementsAre(5));
}

TEST_F(SensorTest, ScanEomTwoMatch)
{
    // When data contains 2 eom, then 2 eom match found.
    auto socket = std::make_shared<FakeSocketReadData>();
    socket->dataToWrite.emplace_back("msg1,msg2,");
    MockSensor sensor(settings, allocator, connection, eventLoop, socket);
    EXPECT_CALL(sensor, publish()).Times(1);
    EXPECT_CALL(sensor, close()).Times(0);
    EXPECT_CALL(sensor, connect(_)).Times(0);

    sensor.call_onReadableCallback(AWS_OP_SUCCESS);
    ASSERT_EQ(sensor.getEomBoundsSize(), 2); // 2 EOM match.
    ASSERT_THAT(sensor.getEomBounds(), ElementsAre(5, 10));
}

TEST_F(SensorTest, ScanEomTwoMatchTwoRead)
{
    // When data contains 2 eom in separate reads, then 2 eom match found.
    auto socket = std::make_shared<FakeSocketReadData>();
    socket->dataToWrite.emplace_back("msg1,");
    socket->dataToWrite.emplace_back("msg2,");
    MockSensor sensor(settings, allocator, connection, eventLoop, socket);
    EXPECT_CALL(sensor, publish()).Times(2);
    EXPECT_CALL(sensor, close()).Times(0);
    EXPECT_CALL(sensor, connect(_)).Times(0);

    sensor.call_onReadableCallback(AWS_OP_SUCCESS);
    ASSERT_EQ(sensor.getEomBoundsSize(), 2); // 2 EOM match.
    ASSERT_THAT(sensor.getEomBounds(), ElementsAre(5, 10));
}

TEST_F(SensorTest, ScanEomTwoReadOneMatch)
{
    // When data contains 1 eom in separate reads, then 1 eom match found.
    auto socket = std::make_shared<FakeSocketReadData>();
    socket->dataToWrite.emplace_back("msg1"); // No EOM.
    socket->dataToWrite.emplace_back("msg2,");
    MockSensor sensor(settings, allocator, connection, eventLoop, socket);
    EXPECT_CALL(sensor, publish()).Times(2);
    EXPECT_CALL(sensor, close()).Times(0);
    EXPECT_CALL(sensor, connect(_)).Times(0);

    sensor.call_onReadableCallback(AWS_OP_SUCCESS);
    ASSERT_EQ(sensor.getEomBoundsSize(), 1); // 1 EOM match.
    ASSERT_THAT(sensor.getEomBounds(), ElementsAre(9));
}

TEST_F(SensorTest, NeedPublishBufferSizeBreach)
{
    // When the bufferSize limit is breached, then we will publish 1 batch.
    settings.bufferTimeMs = 5000;
    settings.bufferSize = 5;

    auto socket = std::make_shared<FakeSocket>();
    MockSensor sensor(settings, allocator, connection, eventLoop, socket);

    sensor.nextPublishTimeout(settings.bufferTimeMs.value());
    sensor.addMessages(settings.bufferSize.value()); // Size is breached.

    size_t bufferSize, numBatches;
    bool doPublish = sensor.needPublish(bufferSize, numBatches);
    ASSERT_TRUE(doPublish);
    ASSERT_EQ(bufferSize, settings.bufferSize.value());
    ASSERT_EQ(numBatches, 1);
}

TEST_F(SensorTest, NeedPublishBufferSizeAndTimeNoBreach)
{
    // When neither bufferSize and bufferTime limits are breached,
    // then we will not publish any batch.
    settings.bufferTimeMs = 5000;
    settings.bufferSize = 5;

    auto socket = std::make_shared<FakeSocket>();
    MockSensor sensor(settings, allocator, connection, eventLoop, socket);

    sensor.nextPublishTimeout(settings.bufferTimeMs.value());
    sensor.addMessages(settings.bufferSize.value() - 1); // One message below limit.

    size_t bufferSize, numBatches;
    bool doPublish = sensor.needPublish(bufferSize, numBatches);
    ASSERT_FALSE(doPublish);
    ASSERT_EQ(bufferSize, settings.bufferSize.value());
    ASSERT_EQ(numBatches, 0);
}

TEST_F(SensorTest, NeedPublishBufferTimeBreach)
{
    // When the bufferTime limit is breached, then we will publish 1 batch.
    settings.bufferTimeMs = 5000;
    settings.bufferSize = 5;

    auto socket = std::make_shared<FakeSocket>();
    MockSensor sensor(settings, allocator, connection, eventLoop, socket);

    sensor.nextPublishTimeout(0);                        // Time is breached.
    sensor.addMessages(settings.bufferSize.value() - 1); // One message below limit.

    size_t bufferSize, numBatches;
    bool doPublish = sensor.needPublish(bufferSize, numBatches);
    ASSERT_TRUE(doPublish);
    ASSERT_EQ(bufferSize, settings.bufferSize.value());
    ASSERT_EQ(numBatches, 1);
}

TEST_F(SensorTest, NeedPublishBufferSize0Breach)
{
    // When the bufferSize limit is 0, then we will publish 1 batch.
    settings.bufferTimeMs = 5000;
    settings.bufferSize = 0;

    auto socket = std::make_shared<FakeSocket>();
    MockSensor sensor(settings, allocator, connection, eventLoop, socket);

    sensor.nextPublishTimeout(settings.bufferTimeMs.value());
    sensor.addMessages(1);

    size_t bufferSize, numBatches;
    bool doPublish = sensor.needPublish(bufferSize, numBatches);
    ASSERT_TRUE(doPublish);
    ASSERT_EQ(bufferSize, 1);
    ASSERT_EQ(numBatches, 1);
}

TEST_F(SensorTest, NeedPublishBufferTime0Breach)
{
    // When the bufferTime limit is 0, then we will publish 1 batch.
    settings.bufferTimeMs = 0;
    settings.bufferSize = 5;

    auto socket = std::make_shared<FakeSocket>();
    MockSensor sensor(settings, allocator, connection, eventLoop, socket);

    sensor.nextPublishTimeout(settings.bufferTimeMs.value());
    sensor.addMessages(1);

    size_t bufferSize, numBatches;
    bool doPublish = sensor.needPublish(bufferSize, numBatches);
    ASSERT_TRUE(doPublish);
    ASSERT_EQ(bufferSize, settings.bufferSize.value());
    ASSERT_EQ(numBatches, 1);
}

TEST_F(SensorTest, NeedPublishBufferSizeMultipleBatches)
{
    // When the number of buffered messages is twice the bufferSize,
    // then we will publish 2 batches.
    settings.bufferTimeMs = 5000;
    settings.bufferSize = 5;

    auto socket = std::make_shared<FakeSocket>();
    MockSensor sensor(settings, allocator, connection, eventLoop, socket);

    sensor.nextPublishTimeout(settings.bufferTimeMs.value());
    sensor.addMessages(settings.bufferSize.value() * 2); // Add 2 X bufferSize messages.

    size_t bufferSize, numBatches;
    bool doPublish = sensor.needPublish(bufferSize, numBatches);
    ASSERT_TRUE(doPublish);
    ASSERT_EQ(bufferSize, settings.bufferSize.value());
    ASSERT_EQ(numBatches, 2);
}

TEST_F(SensorTest, NeedPublishReadBufferFull)
{
    // When the read buffer is full and we have at least 1 message,
    // then we will publish 1 batch.
    settings.bufferTimeMs = 5000;
    settings.bufferSize = 5;
    settings.bufferCapacity = 1024;

    auto socket = std::make_shared<FakeSocket>();
    MockSensor sensor(settings, allocator, connection, eventLoop, socket);

    sensor.nextPublishTimeout(settings.bufferTimeMs.value()); // Time is not breached.
    sensor.addMessages(1);                                    // Messages are below limit.
    sensor.writeReadBuf(settings.bufferCapacity.value());     // Fill read buffer to capacity.

    size_t bufferSize, numBatches;
    bool doPublish = sensor.needPublish(bufferSize, numBatches);
    ASSERT_TRUE(doPublish);
    ASSERT_EQ(bufferSize, settings.bufferSize.value());
    ASSERT_EQ(numBatches, 1);
}

TEST_F(SensorTest, NeedPublishDiscardReadBuffer)
{
    // When the read buffer is full and we haven't found any end of message delimiters,
    // then we will not publish a batch and discard unpublished data.
    settings.bufferTimeMs = 5000;
    settings.bufferSize = 5;
    settings.bufferCapacity = 1024;

    auto socket = std::make_shared<FakeSocket>();
    MockSensor sensor(settings, allocator, connection, eventLoop, socket);

    sensor.nextPublishTimeout(settings.bufferTimeMs.value()); // Time is not breached.
    sensor.addMessages(0);                                    // No messages.
    sensor.writeReadBuf(settings.bufferCapacity.value());     // Fill read buffer to capacity.

    size_t bufferSize, numBatches;
    bool doPublish = sensor.needPublish(bufferSize, numBatches);
    ASSERT_FALSE(doPublish);
    ASSERT_EQ(sensor.getReadBufLen(), 0); // Discard data.
    ASSERT_EQ(bufferSize, settings.bufferSize.value());
    ASSERT_EQ(numBatches, 0);
}

TEST_F(SensorTest, NeedPublishNoMessages)
{
    // When buffering is disabled and there are no messages, then we will not publish a batch.
    settings.bufferTimeMs = 0;
    settings.bufferSize = 0;

    auto socket = std::make_shared<FakeSocket>();
    MockSensor sensor(settings, allocator, connection, eventLoop, socket);

    sensor.nextPublishTimeout(settings.bufferTimeMs.value()); // Time is not breached.
    sensor.addMessages(0);                                    // No messages.

    size_t bufferSize, numBatches;
    bool doPublish = sensor.needPublish(bufferSize, numBatches);
    ASSERT_FALSE(doPublish);
    ASSERT_EQ(bufferSize, 0);
    ASSERT_EQ(numBatches, 0);
}
