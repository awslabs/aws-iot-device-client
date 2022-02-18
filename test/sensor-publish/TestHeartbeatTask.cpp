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

        // Initialize and start the event loop.
        eventLoop = aws_event_loop_new_default(aws_default_allocator(), aws_high_res_clock_get_ticks);
        aws_event_loop_run(eventLoop);
    }

    void TearDown() override { aws_event_loop_destroy(eventLoop); }

    SensorState state;
    PlainConfig::SensorPublish::SensorSettings settings;
    std::shared_ptr<Aws::Crt::Mqtt::MqttConnection> connection;
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

    aws_event_loop_stop(eventLoop);
    aws_event_loop_wait_for_stop_completion(eventLoop);
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

    aws_event_loop_stop(eventLoop);
    aws_event_loop_wait_for_stop_completion(eventLoop);
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

    aws_event_loop_stop(eventLoop);
    aws_event_loop_wait_for_stop_completion(eventLoop);
}
