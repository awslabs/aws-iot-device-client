// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/sensor-publish/SensorPublishFeature.h"
#include "gtest/gtest.h"

#include <aws/common/allocator.h>
#include <aws/common/clock.h>
#include <aws/common/error.h>
#include <aws/crt/Types.h>
#include <aws/crt/io/EventLoopGroup.h>
#include <aws/crt/mqtt/MqttClient.h>
#include <aws/io/event_loop.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace Aws::Iot;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::SensorPublish;

class FakeSharedCrtResourceManager : public SharedCrtResourceManager
{
  public:
    FakeSharedCrtResourceManager()
    {
        allocator = aws_default_allocator();
        eventLoop = aws_event_loop_new_default(allocator, aws_high_res_clock_get_ticks);
    }

    ~FakeSharedCrtResourceManager() { aws_event_loop_destroy(eventLoop); }

    std::shared_ptr<Aws::Crt::Mqtt::MqttConnection> getConnection() override { return connection; }

    aws_event_loop *getNextEventLoop() override { return eventLoop; }

    aws_allocator *getAllocator() override { return allocator; }

    std::unique_ptr<Aws::Crt::ApiHandle> apiHandle;
    aws_allocator *allocator;
    std::shared_ptr<Aws::Crt::Mqtt::MqttConnection> connection; // No connection.
    aws_event_loop *eventLoop;
};

class FakeNotifier : public ClientBaseNotifier
{
  public:
    void onEvent(Feature *feature, ClientBaseEventNotification notification) override
    {
        switch (notification)
        {
            case ClientBaseEventNotification::FEATURE_STARTED:
                ++count_started;
                break;
            case ClientBaseEventNotification::FEATURE_STOPPED:
                ++count_stopped;
                break;
        }
    }

    void onError(Feature *feature, ClientBaseErrorNotification notification, const std::string &message) override {}

    int count_started{0};
    int count_stopped{0};
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

class FakeSensor : public Sensor
{
  public:
    // Simplified constructor to make obtaining instances easy.
    FakeSensor(
        const PlainConfig::SensorPublish::SensorSettings &settings,
        std::shared_ptr<SharedCrtResourceManager> manager)
        : Sensor(
              settings,
              manager->getAllocator(),
              manager->getConnection(),
              manager->getNextEventLoop(),
              std::make_shared<FakeSocket>())
    {
    }

    int start() override { return Feature::SUCCESS; }

    int stop() override { return Feature::SUCCESS; }
};

class SensorPublishFeatureTest : public ::testing::Test
{
  public:
    void SetUp() override
    {
        // Configure settings used by Sensor.
        {
            PlainConfig::SensorPublish::SensorSettings settings;
            settings.name = "my-sensor-01";
            settings.addr = "my-sensor-server-01";
            settings.mqttTopic = "my-sensor-data-01";
            settings.eomDelimiter = "[,]+";
            settings.bufferCapacity = 1024;
            config.sensorPublish.settings.push_back(settings);
        }
        {
            PlainConfig::SensorPublish::SensorSettings settings;
            settings.name = "my-sensor-02";
            settings.addr = "my-sensor-server-02";
            settings.mqttTopic = "my-sensor-data-02";
            settings.eomDelimiter = "[,]+";
            settings.bufferCapacity = 1024;
            config.sensorPublish.settings.push_back(settings);
        }

        // Configure resource manager.
        manager = std::make_shared<FakeSharedCrtResourceManager>();

        // Configure notifier.
        notifier = std::make_shared<FakeNotifier>();
    }

    void TearDown() override {}

    PlainConfig config;
    std::shared_ptr<FakeSharedCrtResourceManager> manager;
    std::shared_ptr<FakeNotifier> notifier;
};

TEST_F(SensorPublishFeatureTest, GetName)
{
    SensorPublishFeature feature;
    ASSERT_EQ(feature.getName(), "Sensor Publish");
}

class MockSensorPublishFeature : public SensorPublishFeature
{
  public:
    std::unique_ptr<Sensor> createSensor(
        const PlainConfig::SensorPublish::SensorSettings &settings,
        aws_allocator *allocator,
        std::shared_ptr<Aws::Crt::Mqtt::MqttConnection> connection,
        aws_event_loop *eventLoop) const override
    {
        // Returns FakeSensor with no-op start and stop.
        return std::unique_ptr<FakeSensor>(new FakeSensor(settings, mResourceManager));
    }
};

TEST_F(SensorPublishFeatureTest, InitSensorSuccess)
{
    // Feature initializes one Sensor per entry in config.
    MockSensorPublishFeature feature;

    int result = feature.init(manager, notifier, config);
    ASSERT_EQ(result, Feature::SUCCESS);
    ASSERT_EQ(feature.getSensorsSize(), 2); // config.sensorPublish.settings.size()
}

TEST_F(SensorPublishFeatureTest, InitSensorDisabled)
{
    // When a Sensor entry is disabled in config,
    // then the entry is not added to the list of sensors.
    config.sensorPublish.settings[1].enabled = false;

    MockSensorPublishFeature feature;

    int result = feature.init(manager, notifier, config);
    ASSERT_EQ(result, Feature::SUCCESS);
    ASSERT_EQ(feature.getSensorsSize(), 1); // config.sensorPublish.settings.size()-1
}

class MockSensorPublishFeatureCreateSensorThrows : public SensorPublishFeature
{
  public:
    std::unique_ptr<Sensor> createSensor(
        const PlainConfig::SensorPublish::SensorSettings &settings,
        aws_allocator *allocator,
        std::shared_ptr<Aws::Crt::Mqtt::MqttConnection> connection,
        aws_event_loop *eventLoop) const override
    {
        throw std::runtime_error{"Sensor constructor throws"};
    }
};

TEST_F(SensorPublishFeatureTest, InitSensorThrows)
{
    // When a Sensor constructor throws an exception,
    // then the entry is not added to the list of sensors.
    config.sensorPublish.settings[1].enabled = false;

    MockSensorPublishFeatureCreateSensorThrows feature;

    int result = feature.init(manager, notifier, config);
    ASSERT_EQ(result, Feature::SUCCESS);
    ASSERT_EQ(feature.getSensorsSize(), 0); // All entries throw exception.
}

TEST_F(SensorPublishFeatureTest, StartSensorSuccess)
{
    // After calling start, feature start notification is sent.
    MockSensorPublishFeature feature;

    int result1 = feature.init(manager, notifier, config);
    ASSERT_EQ(result1, Feature::SUCCESS);

    int result2 = feature.start();
    ASSERT_EQ(result2, Feature::SUCCESS);
    ASSERT_EQ(notifier->count_started, 1);
    ASSERT_EQ(notifier->count_stopped, 0);
}

TEST_F(SensorPublishFeatureTest, StopSensorSuccess)
{
    // After calling stop, feature stop notification is sent.
    MockSensorPublishFeature feature;

    int result1 = feature.init(manager, notifier, config);
    ASSERT_EQ(result1, Feature::SUCCESS);

    int result2 = feature.stop();
    ASSERT_EQ(result2, Feature::SUCCESS);
    ASSERT_EQ(notifier->count_started, 0);
    ASSERT_EQ(notifier->count_stopped, 1);
}
