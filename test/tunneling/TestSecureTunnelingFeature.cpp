// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/tunneling/SecureTunnelingContext.h"
#include "../../source/tunneling/SecureTunnelingFeature.h"
#include "inttypes.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <aws/iotsecuretunneling/IotSecureTunnelingClient.h>
#include <aws/iotsecuretunneling/SubscribeToTunnelsNotifyRequest.h>

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

class FakeSecureTunnelContext : public SecureTunnelingContext
{
  public:
    FakeSecureTunnelContext() : SecureTunnelingContext() {}
    ~FakeSecureTunnelContext() = default;
    bool ConnectToSecureTunnel() override { return true; }
    bool IsDuplicateNotification(const SecureTunnelingNotifyResponse &response) override { return true; }
};

class MockSecureTunnelingFeature : public SecureTunnelingFeature
{
  public:
    MockSecureTunnelingFeature() : SecureTunnelingFeature() {}
    MOCK_METHOD(
        std::unique_ptr<SecureTunnelingContext>,
        createContext,
        (const std::string &accessToken, const std::string &region, const uint16_t &port),
        (override));
    MOCK_METHOD(std::shared_ptr<AbstractIotSecureTunnelingClient>, createClient, (), (override));
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
    void SetUp() override
    {
        thingName = Aws::Crt::String("thing-name value");
        secureTunnelingFeature = shared_ptr<MockSecureTunnelingFeature>(new MockSecureTunnelingFeature());
        manager = shared_ptr<SharedCrtResourceManager>(new SharedCrtResourceManager());
        mockClient = shared_ptr<MockIotSecureTunnelingClient>(new MockIotSecureTunnelingClient());
        notifier = shared_ptr<MockNotifier>(new MockNotifier());
        fakeContext = unique_ptr<FakeSecureTunnelContext>(new FakeSecureTunnelContext());
        response = unique_ptr<SecureTunnelingNotifyResponse>(new SecureTunnelingNotifyResponse());
        config = getConfig();
    }
    Aws::Crt::String thingName;
    shared_ptr<MockIotSecureTunnelingClient> mockClient;
    shared_ptr<MockSecureTunnelingFeature> secureTunnelingFeature;
    shared_ptr<SharedCrtResourceManager> manager;
    shared_ptr<MockNotifier> notifier;
    unique_ptr<FakeSecureTunnelContext> fakeContext;
    unique_ptr<SecureTunnelingNotifyResponse> response;
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

TEST_F(TestSecureTunnelingFeature, CreateSSHContextHappy)
{
    /**
     * Invokes NotifyResponse handler for SSH service, verifies SecureTunnelContext params
     */
    string accessToken = "12345";
    string region = "us-west-2";
    uint16_t port = 22;
    Aws::Crt::Vector<Aws::Crt::String> services;
    services.push_back("SSH");

    response->ClientMode = "destination";
    response->Services = services;
    response->ClientAccessToken = accessToken.c_str();
    response->Region = region.c_str();

    EXPECT_CALL(*secureTunnelingFeature, createContext(StrEq(accessToken), StrEq(region), Eq(port)))
        .Times(1)
        .WillOnce(Return(ByMove(std::move(fakeContext))));
    EXPECT_CALL(*secureTunnelingFeature, createClient()).Times(1).WillOnce(Return(mockClient));
    EXPECT_CALL(*mockClient, SubscribeToTunnelsNotify(ThingNameEq(thingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(DoAll(InvokeArgument<2>(response.get(), 0), InvokeArgument<3>(0)));
    EXPECT_CALL(*notifier, onEvent(_, _)).Times(2);
    secureTunnelingFeature->init(manager, notifier, config);
    secureTunnelingFeature->start();
    secureTunnelingFeature->stop();
}

TEST_F(TestSecureTunnelingFeature, CreateVNCContextHappy)
{
    /**
     * Invokes NotifyResponse handler for VNC service, verifies SecureTunnelContext params
     */
    string accessToken = "12345";
    string region = "us-west-2";
    uint16_t port = 5900;
    Aws::Crt::Vector<Aws::Crt::String> services;
    services.push_back("VNC");

    response->ClientMode = "destination";
    response->Services = services;
    response->ClientAccessToken = accessToken.c_str();
    response->Region = region.c_str();

    EXPECT_CALL(*secureTunnelingFeature, createContext(StrEq(accessToken), StrEq(region), Eq(port)))
        .Times(1)
        .WillOnce(Return(ByMove(std::move(fakeContext))));
    EXPECT_CALL(*secureTunnelingFeature, createClient()).Times(1).WillOnce(Return(mockClient));
    EXPECT_CALL(*mockClient, SubscribeToTunnelsNotify(ThingNameEq(thingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(DoAll(InvokeArgument<2>(response.get(), 0), InvokeArgument<3>(0)));
    EXPECT_CALL(*notifier, onEvent(_, _)).Times(2);
    secureTunnelingFeature->init(manager, notifier, config);
    secureTunnelingFeature->start();
    secureTunnelingFeature->stop();
}

TEST_F(TestSecureTunnelingFeature, ResponseNULL)
{
    /**
     * Invokes NotifyResponse handler with null response
     * Expect no creation of SecureTunnelContext
     */
    EXPECT_CALL(*secureTunnelingFeature, createContext(_, _, _)).Times(0);
    EXPECT_CALL(*secureTunnelingFeature, createClient()).Times(1).WillOnce(Return(mockClient));
    EXPECT_CALL(*mockClient, SubscribeToTunnelsNotify(ThingNameEq(thingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(DoAll(InvokeArgument<2>(nullptr, 1), InvokeArgument<3>(0)));
    EXPECT_CALL(*notifier, onEvent(_, _)).Times(2);
    secureTunnelingFeature->init(manager, notifier, config);
    secureTunnelingFeature->start();
    secureTunnelingFeature->stop();
}

TEST_F(TestSecureTunnelingFeature, ResponseIoError)
{
    /**
     * Invokes NotifyResponse handler with error code 1
     * Expect no creation of SecureTunnelContext
     */

    string accessToken = "12345";
    string region = "us-west-2";
    Aws::Crt::Vector<Aws::Crt::String> services;
    services.push_back("SSH");

    response->ClientMode = "destination";
    response->Services = services;
    response->ClientAccessToken = accessToken.c_str();
    response->Region = region.c_str();

    EXPECT_CALL(*secureTunnelingFeature, createContext(_, _, _)).Times(0);
    EXPECT_CALL(*secureTunnelingFeature, createClient()).Times(1).WillOnce(Return(mockClient));
    EXPECT_CALL(*mockClient, SubscribeToTunnelsNotify(ThingNameEq(thingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(DoAll(InvokeArgument<2>(response.get(), 1), InvokeArgument<3>(0)));
    EXPECT_CALL(*notifier, onEvent(_, _)).Times(2);
    secureTunnelingFeature->init(manager, notifier, config);
    secureTunnelingFeature->start();
    secureTunnelingFeature->stop();
}

TEST_F(TestSecureTunnelingFeature, DuplicateResponse)
{
    /**
     * Invokes NotifyResponse with duplicate responses
     * Expect single SecureTunnelingContext
     */
    string accessToken = "12345";
    string region = "us-west-2";
    uint16_t port = 22;
    Aws::Crt::Vector<Aws::Crt::String> services;
    services.push_back("SSH");

    response->ClientMode = "destination";
    response->Services = services;
    response->ClientAccessToken = accessToken.c_str();
    response->Region = region.c_str();

    EXPECT_CALL(*secureTunnelingFeature, createContext(StrEq(accessToken), StrEq(region), Eq(port)))
        .Times(1)
        .WillOnce(Return(ByMove(std::move(fakeContext))));
    EXPECT_CALL(*secureTunnelingFeature, createClient()).Times(1).WillOnce(Return(mockClient));
    EXPECT_CALL(*mockClient, SubscribeToTunnelsNotify(ThingNameEq(thingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(
            DoAll(InvokeArgument<2>(response.get(), 0), InvokeArgument<2>(response.get(), 0), InvokeArgument<3>(0)));
    EXPECT_CALL(*notifier, onEvent(_, _)).Times(2);
    secureTunnelingFeature->init(manager, notifier, config);
    secureTunnelingFeature->start();
    secureTunnelingFeature->stop();
}

TEST_F(TestSecureTunnelingFeature, MultipleServices)
{
    /**
     * Invokes NotifyResponse with multiple services
     * Expect no SecureTunnelContext (multi-port tunneling unsupported on device client)
     */

    string accessToken = "12345";
    string region = "us-west-2";
    Aws::Crt::Vector<Aws::Crt::String> services;
    services.push_back("SSH");
    services.push_back("VNC");

    response->ClientMode = "destination";
    response->Services = services;
    response->ClientAccessToken = accessToken.c_str();
    response->Region = region.c_str();

    EXPECT_CALL(*secureTunnelingFeature, createContext(_, _, _)).Times(0);
    EXPECT_CALL(*secureTunnelingFeature, createClient()).Times(1).WillOnce(Return(mockClient));
    EXPECT_CALL(*mockClient, SubscribeToTunnelsNotify(ThingNameEq(thingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(DoAll(InvokeArgument<2>(response.get(), 1), InvokeArgument<3>(0)));
    EXPECT_CALL(*notifier, onEvent(_, _)).Times(2);
    secureTunnelingFeature->init(manager, notifier, config);
    secureTunnelingFeature->start();
    secureTunnelingFeature->stop();
}

TEST_F(TestSecureTunnelingFeature, UnsupportedService)
{
    /**
     * Invokes NotifyResponse with zero services
     * Expect no SecureTunnelContext
     */

    string accessToken = "12345";
    string region = "us-west-2";
    Aws::Crt::Vector<Aws::Crt::String> services;
    services.push_back("UnsupportedService");

    response->ClientMode = "destination";
    response->Services = services;
    response->ClientAccessToken = accessToken.c_str();
    response->Region = region.c_str();

    EXPECT_CALL(*secureTunnelingFeature, createContext(_, _, _)).Times(0);
    EXPECT_CALL(*secureTunnelingFeature, createClient()).Times(1).WillOnce(Return(mockClient));
    EXPECT_CALL(*mockClient, SubscribeToTunnelsNotify(ThingNameEq(thingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(DoAll(InvokeArgument<2>(response.get(), 1), InvokeArgument<3>(0)));
    EXPECT_CALL(*notifier, onEvent(_, _)).Times(2);
    secureTunnelingFeature->init(manager, notifier, config);
    secureTunnelingFeature->start();
    secureTunnelingFeature->stop();
}

TEST_F(TestSecureTunnelingFeature, NoServices)
{
    /**
     * Invokes NotifyResponse with zero services
     * Expect no SecureTunnelContext
     */

    string accessToken = "12345";
    string region = "us-west-2";
    Aws::Crt::Vector<Aws::Crt::String> services;

    response->ClientMode = "destination";
    response->Services = services;
    response->ClientAccessToken = accessToken.c_str();
    response->Region = region.c_str();

    EXPECT_CALL(*secureTunnelingFeature, createContext(_, _, _)).Times(0);
    EXPECT_CALL(*secureTunnelingFeature, createClient()).Times(1).WillOnce(Return(mockClient));
    EXPECT_CALL(*mockClient, SubscribeToTunnelsNotify(ThingNameEq(thingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(DoAll(InvokeArgument<2>(response.get(), 1), InvokeArgument<3>(0)));
    EXPECT_CALL(*notifier, onEvent(_, _)).Times(2);
    secureTunnelingFeature->init(manager, notifier, config);
    secureTunnelingFeature->start();
    secureTunnelingFeature->stop();
}

TEST_F(TestSecureTunnelingFeature, SourceMode)
{
    /**
     * Invokes NotifyResponse with in source mode
     * Expect no SecureTunnelContext source ClientMode not supported on Device Client
     */

    string accessToken = "12345";
    string region = "us-west-2";
    Aws::Crt::Vector<Aws::Crt::String> services;
    services.push_back("SSH");

    response->ClientMode = "source";
    response->Services = services;
    response->ClientAccessToken = accessToken.c_str();
    response->Region = region.c_str();

    EXPECT_CALL(*secureTunnelingFeature, createContext(_, _, _)).Times(0);
    EXPECT_CALL(*secureTunnelingFeature, createClient()).Times(1).WillOnce(Return(mockClient));
    EXPECT_CALL(*mockClient, SubscribeToTunnelsNotify(ThingNameEq(thingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(DoAll(InvokeArgument<2>(response.get(), 1), InvokeArgument<3>(0)));
    EXPECT_CALL(*notifier, onEvent(_, _)).Times(2);
    secureTunnelingFeature->init(manager, notifier, config);
    secureTunnelingFeature->start();
    secureTunnelingFeature->stop();
}
