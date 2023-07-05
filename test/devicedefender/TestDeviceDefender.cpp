// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/devicedefender/DeviceDefenderFeature.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace std;
using namespace testing;
using namespace Aws;
using namespace Aws::Crt;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::DeviceDefender;

PlainConfig getSimpleDDConfig()
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "key": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test-file",
    "thing-name": "thing-name value",
    "logging": {
        "level": "ERROR",
        "type": "file",
        "file": "./aws-iot-device-client.log"
    },
    "device-defender":	{
        "enabled":	true,
        "interval": 300
    }
})";

    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    return config;
}

class MockNotifier : public Aws::Iot::DeviceClient::ClientBaseNotifier
{
  public:
    MOCK_METHOD(
        void,
        onEvent,
        (Aws::Iot::DeviceClient::Feature * feature, Aws::Iot::DeviceClient::ClientBaseEventNotification notification),
        (override));
    MOCK_METHOD(
        void,
        onError,
        (Aws::Iot::DeviceClient::Feature * feature,
         Aws::Iot::DeviceClient::ClientBaseErrorNotification notification,
         const std::string &message),
        (override));
};

class MockReportTask : public ReportTaskWrapper
{
  public:
    MockReportTask() : ReportTaskWrapper() {}
    MOCK_METHOD(int, StartTask, (), (override));
    MOCK_METHOD(void, StopTask, (), (override));
};

class MockDDFeature : public DeviceDefenderFeature
{
  public:
    MockDDFeature() : DeviceDefenderFeature() {}
    string getThingName() { return this->thingName; }
    int getInterval() { return this->interval; }
    MOCK_METHOD(shared_ptr<AbstractReportTask>, createReportTask, (), (override));
    MOCK_METHOD(void, subscribeToTopicFilter, (), (override));
    MOCK_METHOD(void, unsubscribeToTopicFilter, (), (override));
};

class TestDeviceDefender : public testing::Test
{
  public:
    void SetUp()
    {
        deviceDefender = unique_ptr<MockDDFeature>(new MockDDFeature());
        manager = shared_ptr<SharedCrtResourceManager>(new SharedCrtResourceManager());
        // Initializing allocator, so we can use CJSON lib from SDK in our unit tests.
        manager->initializeAllocator();

        notifier = shared_ptr<MockNotifier>(new MockNotifier());
        task = shared_ptr<MockReportTask>(new MockReportTask());
        config = getSimpleDDConfig();
    }
    unique_ptr<MockDDFeature> deviceDefender;
    shared_ptr<SharedCrtResourceManager> manager;
    shared_ptr<MockNotifier> notifier;
    shared_ptr<MockReportTask> task;
    PlainConfig config;
};

TEST_F(TestDeviceDefender, GetName)
{
    /**
     * Simple test for GetName
     */
    ASSERT_STREQ(deviceDefender->getName().c_str(), "Device Defender");
}

TEST_F(TestDeviceDefender, Init)
{
    /**
     * Simple init test
     * Inject Mocks via init, verify Thing Name and Interval
     */
    ASSERT_EQ(deviceDefender->init(manager, notifier, config), 0);
    ASSERT_STREQ(deviceDefender->getThingName().c_str(), "thing-name value");
    ASSERT_EQ(deviceDefender->getInterval(), 300);
}

TEST_F(TestDeviceDefender, Start)
{
    /**
     * Simple Start test
     * Inject Report Task, init, and start, verify Thing Name and Interval
     * Verify calls on Mocks and Assert Start returns 0
     */
    EXPECT_CALL(*task, StartTask()).Times(1).WillOnce(Return(0));
    EXPECT_CALL(*deviceDefender, createReportTask()).Times(1).WillOnce(Return(task));
    EXPECT_CALL(*deviceDefender, subscribeToTopicFilter()).Times(1).WillOnce(Return());
    EXPECT_CALL(*notifier, onEvent(_, _)).Times(1).WillOnce(Return());

    ASSERT_EQ(deviceDefender->init(manager, notifier, config), 0);
    ASSERT_EQ(deviceDefender->start(), 0);
    ASSERT_STREQ(deviceDefender->getThingName().c_str(), "thing-name value");
    ASSERT_EQ(deviceDefender->getInterval(), 300);
}

TEST_F(TestDeviceDefender, StartStop)
{
    /**
     * Start and Stop test
     * Inject Report Task, init, and start, verify Thing Name and Interval
     * Verify calls on Mocks and Assert Start and Stop return 0
     */
    EXPECT_CALL(*task, StartTask()).Times(1).WillOnce(Return(0));
    EXPECT_CALL(*deviceDefender, createReportTask()).Times(1).WillOnce(Return(task));
    EXPECT_CALL(*deviceDefender, subscribeToTopicFilter()).Times(1).WillOnce(Return());
    EXPECT_CALL(*notifier, onEvent(_, _)).Times(2).WillRepeatedly(Return());
    EXPECT_CALL(*task, StopTask()).Times(1);
    EXPECT_CALL(*deviceDefender, unsubscribeToTopicFilter()).Times(1).WillOnce(Return());

    ASSERT_EQ(deviceDefender->init(manager, notifier, config), 0);
    ASSERT_EQ(deviceDefender->start(), 0);
    ASSERT_EQ(deviceDefender->stop(), 0);
    ASSERT_STREQ(deviceDefender->getThingName().c_str(), "thing-name value");
    ASSERT_EQ(deviceDefender->getInterval(), 300);
}
