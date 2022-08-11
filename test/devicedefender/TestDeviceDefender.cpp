// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/devicedefender/DeviceDefenderFeature.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace std;
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

class TestDeviceDefender : public testing::Test
{
  public:
    void SetUp()
    {
        deviceDefender = unique_ptr<DeviceDefenderFeature>(new DeviceDefenderFeature());
        manager = shared_ptr<SharedCrtResourceManager>(new SharedCrtResourceManager());
        notifier = shared_ptr<MockNotifier>(new MockNotifier());
        config = getSimpleDDConfig();
    }
    unique_ptr<DeviceDefenderFeature> deviceDefender;
    shared_ptr<SharedCrtResourceManager> manager;
    shared_ptr<MockNotifier> notifier;
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
     */
    ASSERT_EQ(deviceDefender->init(manager, notifier, config), 0);
}
