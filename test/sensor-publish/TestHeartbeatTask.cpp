// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/sensor-publish/HeartbeatTask.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <aws/common/allocator.h>
#include <aws/common/clock.h>
#include <aws/crt/Types.h>
#include <aws/crt/mqtt/MqttClient.h>
#include <aws/io/event_loop.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

using namespace Aws::Iot;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::SensorPublish;

using ::testing::_;
using ::testing::AtLeast;

void wait(std::int64_t delay_ms)
{
    std::thread t([&delay_ms]() { std::this_thread::sleep_for(std::chrono::milliseconds{delay_ms}); });
    t.join();
}

class HeartbeatTaskTest : public ::testing::Test
{
  public:
    void SetUp() override
    {
        // Set sensor state machine as connected.
        state = SensorState::Connected;

        // Configure settings used by HeartbeatTask.
        settings.name = "my-sensor";
        settings.mqttHeartbeatTopic = "my-sensor-heartbeat";
        settings.heartbeatTimeSec = 0; // Publish without delay.

        // Initialize event loop group and get an event loop from it.
        aws_event_loop_group_options elg_options;
        AWS_ZERO_STRUCT(elg_options);
        elg_options.loop_count = 1;
        elg_options.shutdown_options = nullptr;
        eventLoopGroup = aws_event_loop_group_new(aws_default_allocator(), &elg_options);
        eventLoop = aws_event_loop_group_get_next_loop(eventLoopGroup);
    }

    void TearDown() override { aws_event_loop_group_release(eventLoopGroup); }

    SensorState state;
    PlainConfig::SensorPublish::SensorSettings settings;
    std::shared_ptr<Aws::Crt::Mqtt::MqttConnection> connection;
    aws_event_loop_group *eventLoopGroup;
    aws_event_loop *eventLoop;
};

struct MockHeartbeatTask : public HeartbeatTask
{
  public:
    MockHeartbeatTask(
        const SensorState &state,
        const PlainConfig::SensorPublish::SensorSettings &settings,
        std::shared_ptr<Aws::Crt::Mqtt::MqttConnection> connection,
        aws_event_loop *eventLoop)
        : HeartbeatTask(state, settings, connection, eventLoop)
    {
    }

    MOCK_METHOD(void, publish, (), (override));
};

TEST_F(HeartbeatTaskTest, TopicNotSpecified)
{
    // When a heartbeat topic is not specified, then task is never started.
    settings.mqttHeartbeatTopic = std::string{};

    MockHeartbeatTask task(state, settings, connection, eventLoop);
    EXPECT_CALL(task, publish()).Times(0);

    task.start();
    ASSERT_FALSE(task.started());

    wait(100);

    task.stop();
    ASSERT_FALSE(task.started());
}

TEST_F(HeartbeatTaskTest, TaskStartedAndNoHeartbeat)
{
    // When sensor state is not connected, the task is started and but no heartbeat is published.
    state = SensorState::NotConnected;

    MockHeartbeatTask task(state, settings, connection, eventLoop);
    EXPECT_CALL(task, publish()).Times(0);

    task.start();
    ASSERT_TRUE(task.started());

    wait(100);

    task.stop();
    ASSERT_FALSE(task.started());
}

TEST_F(HeartbeatTaskTest, TaskStartedAndHeartbeat)
{
    // When sensor state is connected, the task is started and heartbeat is published.
    state = SensorState::Connected;

    MockHeartbeatTask task(state, settings, connection, eventLoop);
    EXPECT_CALL(task, publish()).Times(AtLeast(1));

    task.start();
    ASSERT_TRUE(task.started());

    wait(100);

    task.stop();
    ASSERT_FALSE(task.started());
}
