// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/tunneling/SecureTunnelingFeature.h"
#include "aws/iotsecuretunneling/IotSecureTunnelingClient.h"
#include "aws/iotsecuretunneling/SubscribeToTunnelsNotifyRequest.h"
#include "../../source/tunneling/SecureTunnelingContext.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "inttypes.h"

using namespace testing;
using namespace std;
using namespace Aws;
using namespace Aws::Crt;
using namespace Aws::Iot;
using namespace Aws::Iotsecuretunneling;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::SecureTunneling;

PlainConfig getConfig()
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test/AmazonRootCA1.pem",
    "key": "/tmp/aws-iot-device-client-test-file",
    "thing-name": "thing-name value",
    "tunneling": {
        "enabled": true
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

class MockSecureTunnelingFeature : public SecureTunnelingFeature
{
  public:
    MockSecureTunnelingFeature() : SecureTunnelingFeature() {}
    MOCK_METHOD(
        void,
        getContext,
        (std::unique_ptr<SecureTunnelingContext> &context,
         const std::string &accessToken,
         const std::string &region,
         const uint16_t port),
        (override));
    MOCK_METHOD(std::shared_ptr<AbstractIotSecureTunnelingClient>, getClient, (), (override));
};

class MockIotSecureTunnelingClient : public AbstractIotSecureTunnelingClient
{
  public:
    MockIotSecureTunnelingClient() = default;
    MOCK_METHOD(
        void,
        SubscribeToTunnelsNotify,
        (const Iotsecuretunneling::SubscribeToTunnelsNotifyRequest &request,
         Aws::Crt::Mqtt::QOS qos,
         const Iotsecuretunneling::OnSubscribeToTunnelsNotifyResponse &handler,
         const Iotsecuretunneling::OnSubscribeComplete &onSubAck),
        (override));
};

class TestSecureTunnelingFeature : public testing::Test
{
  public:
    void SetUp() override {
        thingName = Aws::Crt::String("thing-name value");
        secureTunnelingFeature = shared_ptr<MockSecureTunnelingFeature>(new MockSecureTunnelingFeature());
        manager = shared_ptr<SharedCrtResourceManager>(new SharedCrtResourceManager());
        mockClient = shared_ptr<MockIotSecureTunnelingClient>(new MockIotSecureTunnelingClient());
        notifier = shared_ptr<MockNotifier>(new MockNotifier());
        config = getConfig();
    }
    Aws::Crt::String thingName;
    shared_ptr<MockIotSecureTunnelingClient> mockClient;
    shared_ptr<MockSecureTunnelingFeature> secureTunnelingFeature;
    shared_ptr<SharedCrtResourceManager> manager;
    shared_ptr<MockNotifier> notifier;
    PlainConfig config;
};

MATCHER_P(ThingNameEq, ThingName, "Matcher ThingName for all Aws request Objects using Aws::Crt::String")
{
    return arg.ThingName.value() == ThingName;
}

TEST_F(TestSecureTunnelingFeature, GetName)
{
    /**
     * Simple test for getName
     */
    ASSERT_EQ("Secure Tunneling", secureTunnelingFeature->getName());
}

TEST_F(TestSecureTunnelingFeature, Init)
{
    /**
     * Simple init of SecureTunnelingFeature
     */
    ASSERT_EQ(0, secureTunnelingFeature->init(manager, notifier, config));
}

TEST_F(TestSecureTunnelingFeature, StartHappy)
{
    /**
     * Simple start of SecureTunnelingFeature
     * Injects a mock IotSecureTunnelingClient, invokes callback and handler with (nullptr, 1)
     */
    EXPECT_CALL(*secureTunnelingFeature, getClient()).Times(1).WillOnce(Return(mockClient));
    EXPECT_CALL(*mockClient, SubscribeToTunnelsNotify(ThingNameEq(thingName),AWS_MQTT_QOS_AT_LEAST_ONCE,_,_)).Times(1).WillOnce(DoAll(InvokeArgument<2>(nullptr, 1)));
    EXPECT_CALL(*notifier, onEvent(_,_)).Times(1);
    secureTunnelingFeature->init(manager, notifier, config);
    secureTunnelingFeature->start();
}

TEST_F(TestSecureTunnelingFeature, CreateSSHContextHappy)
{
    auto *response = new SecureTunnelingNotifyResponse();
    response->ClientMode = "destination";
    response->Services->push_back("SSH");
    response->ClientAccessToken = "12345";
    response->Region = "us-west-2";

    EXPECT_CALL(*secureTunnelingFeature, getContext(_, "12345", "us-west-2", "22")).Times(1);

    EXPECT_CALL(*secureTunnelingFeature, getClient()).Times(1).WillOnce(Return(mockClient));
    EXPECT_CALL(*mockClient, SubscribeToTunnelsNotify(ThingNameEq(thingName),AWS_MQTT_QOS_AT_LEAST_ONCE,_,_)).Times(1).WillOnce(DoAll(InvokeArgument<2>(response, 0)));
    EXPECT_CALL(*notifier, onEvent(_,_)).Times(1);
    secureTunnelingFeature->init(manager, notifier, config);
    secureTunnelingFeature->start();
}

