// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/tunneling/SecureTunnelingContext.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <aws/common/allocator.h>

using namespace testing;
using namespace std;
using namespace Aws;
using namespace Aws::Crt;
using namespace Aws::Crt::Io;
using namespace Aws::Iot;
using namespace Aws::Iotsecuretunneling;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::SecureTunneling;

class MockSecureTunnelingContext : public SecureTunnelingContext
{
  public:
    MockSecureTunnelingContext(
        shared_ptr<SharedCrtResourceManager> manager,
        const Aws::Crt::Optional<std::string> &rootCa,
        const string &accessToken,
        const string &endpoint,
        const int port,
        const OnConnectionShutdownFn &onConnectionShutdown)
        : SecureTunnelingContext(manager, rootCa, accessToken, endpoint, port, onConnectionShutdown)
    {
    }

    MOCK_METHOD(
        std::shared_ptr<SecureTunnelWrapper>,
        CreateSecureTunnel,
        (const Aws::Iotsecuretunneling::OnConnectionComplete &onConnectionComplete,
         const Aws::Iotsecuretunneling::OnConnectionShutdown &onConnectionShutdown,
         const Aws::Iotsecuretunneling::OnSendDataComplete &onSendDataComplete,
         const Aws::Iotsecuretunneling::OnDataReceive &onDataReceive,
         const Aws::Iotsecuretunneling::OnStreamStart &onStreamStart,
         const Aws::Iotsecuretunneling::OnStreamReset &onStreamReset,
         const Aws::Iotsecuretunneling::OnSessionReset &onSessionReset),
        (override));

    MOCK_METHOD(std::shared_ptr<TcpForward>, CreateTcpForward, (), (override));
    MOCK_METHOD(void, DisconnectFromTcpForward, (), (override));
};

class MockSecureTunnel : public SecureTunnelWrapper
{
  public:
    MockSecureTunnel() : SecureTunnelWrapper() {}
    MOCK_METHOD(int, Connect, (), (override));
    MOCK_METHOD(int, Close, (), (override));
    MOCK_METHOD(int, SendData, (const Aws::Crt::ByteCursor &data), (override));
    bool IsValid() override { return true; }
};

class MockTcpForward : public TcpForward
{
  public:
    MockTcpForward(std::shared_ptr<SharedCrtResourceManager> sharedCrtResourceManager, uint16_t port)
        : TcpForward(sharedCrtResourceManager, port)
    {
    }
    MOCK_METHOD(int, Connect, (), (override));
    MOCK_METHOD(int, SendData, (const Crt::ByteCursor &data), (override));
};

class TestSecureTunnelContext : public testing::Test
{
  public:
    void SetUp() override
    {
        manager = shared_ptr<SharedCrtResourceManager>(new SharedCrtResourceManager());
        tunnel = shared_ptr<MockSecureTunnel>(new MockSecureTunnel());
        tcpForward = shared_ptr<MockTcpForward>(new MockTcpForward(manager, port));
        rootCa = "root-ca-value";
        accessToken = "access-token-value";
        endpoint = "endpoint-value";
        port = 5555;
    }
    unique_ptr<MockSecureTunnelingContext> context;
    shared_ptr<MockSecureTunnel> tunnel;
    shared_ptr<MockTcpForward> tcpForward;
    shared_ptr<SharedCrtResourceManager> manager;
    Aws::Crt::Optional<std::string> rootCa;
    string accessToken;
    string endpoint;
    int port;
    OnConnectionShutdownFn onConnectionShutdown;
};

TEST_F(TestSecureTunnelContext, ConnectToSecureTunnelHappy)
{
    /**
     * Create a MockSecureTunnelingContext and inject a MockSecureTunnel
     * Verify ConnectToSecureTunnel returns true
     */
    context = unique_ptr<MockSecureTunnelingContext>(
        new MockSecureTunnelingContext(manager, rootCa, accessToken, endpoint, port, nullptr));

    EXPECT_CALL(*context, CreateSecureTunnel(_, _, _, _, _, _, _)).WillOnce(Return(tunnel));
    EXPECT_CALL(*tunnel, Connect()).WillOnce(Return(0));
    EXPECT_CALL(*tunnel, Close()).WillOnce(Return(0));

    ASSERT_TRUE(context->ConnectToSecureTunnel());
}

TEST_F(TestSecureTunnelContext, ConnectToSecureTunnelMissingAccessToken)
{
    /**
     * Create a MockSecureTunnelingContext with an empty Access Token
     * Verify ConnectToSecureTunnel returns false
     */
    context = unique_ptr<MockSecureTunnelingContext>(
        new MockSecureTunnelingContext(manager, rootCa, "", "12345", port, nullptr));

    ASSERT_FALSE(context->ConnectToSecureTunnel());
}

TEST_F(TestSecureTunnelContext, ConnectToSecureTunnelMissingEndpoint)
{
    /**
     * Create a MockSecureTunnelingContext with an empty endpoint
     * Verify ConnectToSecureTunnel returns false
     */
    context = unique_ptr<MockSecureTunnelingContext>(
        new MockSecureTunnelingContext(manager, rootCa, "12345", "", port, nullptr));

    ASSERT_FALSE(context->ConnectToSecureTunnel());
}

TEST_F(TestSecureTunnelContext, OnStreamStartHappy)
{
    /**
     * Create a MockSecureTunnelingContext and inject a mock SecureTunnel and mock TcpForward
     * Invoke OnStreamStart callback
     * Verify calls on TcpForward, SecureTunnel, and that ConnectToSecureTunnel returns true
     */
    context = unique_ptr<MockSecureTunnelingContext>(
        new MockSecureTunnelingContext(manager, rootCa, accessToken, endpoint, port, nullptr));

    EXPECT_CALL(*context, CreateSecureTunnel(_, _, _, _, _, _, _)).WillOnce(DoAll(InvokeArgument<4>(), Return(tunnel)));
    EXPECT_CALL(*context, CreateTcpForward()).WillOnce(Return(tcpForward));
    EXPECT_CALL(*tcpForward, Connect()).WillOnce(Return(0));
    EXPECT_CALL(*tunnel, Connect()).WillOnce(Return(0));
    EXPECT_CALL(*tunnel, Close()).WillOnce(Return(0));

    ASSERT_TRUE(context->ConnectToSecureTunnel());
}

TEST_F(TestSecureTunnelContext, OnStreamStartInvalidPortLow)
{
    /**
     * Create a MockSecureTunnelContext with invalid (too low) port number and inject a mock SecureTunnel
     * Verify no create TcpForward
     */
    context = unique_ptr<MockSecureTunnelingContext>(
        new MockSecureTunnelingContext(manager, rootCa, accessToken, endpoint, 0, nullptr));

    EXPECT_CALL(*context, CreateSecureTunnel(_, _, _, _, _, _, _)).WillOnce(DoAll(InvokeArgument<4>(), Return(tunnel)));
    EXPECT_CALL(*context, CreateTcpForward()).Times(0);
    EXPECT_CALL(*tunnel, Connect()).WillOnce(Return(0));
    EXPECT_CALL(*tunnel, Close()).WillOnce(Return(0));

    ASSERT_TRUE(context->ConnectToSecureTunnel());
}

TEST_F(TestSecureTunnelContext, OnStreamStartInvalidPortHigh)
{
    /**
     * Create a MockSecureTunnelContext with invalid (too high) port number and inject a mock SecureTunnel
     * Verify no create TcpForward
     */
    context = unique_ptr<MockSecureTunnelingContext>(
        new MockSecureTunnelingContext(manager, rootCa, accessToken, endpoint, 65536, nullptr));

    EXPECT_CALL(*context, CreateSecureTunnel(_, _, _, _, _, _, _)).WillOnce(DoAll(InvokeArgument<4>(), Return(tunnel)));
    EXPECT_CALL(*context, CreateTcpForward()).Times(0);
    EXPECT_CALL(*tunnel, Connect()).WillOnce(Return(0));
    EXPECT_CALL(*tunnel, Close()).WillOnce(Return(0));

    ASSERT_TRUE(context->ConnectToSecureTunnel());
}

TEST_F(TestSecureTunnelContext, OnStreamReset)
{
    /**
     * Create a MockSecureTunnelingContext and inject a MockSecureTunnel and mock TcpForward
     * Invoke OnStreamReset callback
     * Verify calls on tunnel and DisconnectTcpForward, ConnectToSecureTunnel returns true
     */
    context = unique_ptr<MockSecureTunnelingContext>(
        new MockSecureTunnelingContext(manager, rootCa, accessToken, endpoint, port, nullptr));

    EXPECT_CALL(*context, CreateSecureTunnel(_, _, _, _, _, _, _)).WillOnce(DoAll(InvokeArgument<5>(), Return(tunnel)));
    EXPECT_CALL(*context, DisconnectFromTcpForward()).Times(1);
    EXPECT_CALL(*tunnel, Connect()).WillOnce(Return(0));
    EXPECT_CALL(*tunnel, Close()).WillOnce(Return(0));

    ASSERT_TRUE(context->ConnectToSecureTunnel());
}

TEST_F(TestSecureTunnelContext, OnSessionReset)
{
    /**
     * Create a MockSecureTunnelingContext and inject a MockSecureTunnel and mock TcpForward
     * Invoke OnSessionReset callback
     * Verify calls on tunnel and DisconnectTcpForward, ConnectToSecureTunnel returns true
     */
    context = unique_ptr<MockSecureTunnelingContext>(
        new MockSecureTunnelingContext(manager, rootCa, accessToken, endpoint, port, nullptr));

    EXPECT_CALL(*context, CreateSecureTunnel(_, _, _, _, _, _, _)).WillOnce(DoAll(InvokeArgument<6>(), Return(tunnel)));
    EXPECT_CALL(*context, DisconnectFromTcpForward()).Times(1);
    EXPECT_CALL(*tunnel, Connect()).WillOnce(Return(0));
    EXPECT_CALL(*tunnel, Close()).WillOnce(Return(0));

    ASSERT_TRUE(context->ConnectToSecureTunnel());
}

TEST_F(TestSecureTunnelContext, OnDataReceive)
{
    /**
     * Create a MockSecureTunnelingContext and inject a MockSecureTunnel and mock TcpForward
     * Invoke OnDataReceive callback with test data
     * Verify calls on tunnel and TcpForward, ConnectToSecureTunnel returns true
     */
    Crt::ByteBuf data = ByteBufFromCString("Test Data");

    context = unique_ptr<MockSecureTunnelingContext>(
        new MockSecureTunnelingContext(manager, rootCa, accessToken, endpoint, port, nullptr));

    EXPECT_CALL(*context, CreateSecureTunnel(_, _, _, _, _, _, _))
        .WillOnce(DoAll(InvokeArgument<4>(), InvokeArgument<3>(data), Return(tunnel)));
    EXPECT_CALL(*context, CreateTcpForward()).WillOnce(Return(tcpForward));
    EXPECT_CALL(*tcpForward, Connect()).WillOnce(Return(0));
    EXPECT_CALL(*tcpForward, SendData(_)).Times(1);
    EXPECT_CALL(*tunnel, Connect()).WillOnce(Return(0));
    EXPECT_CALL(*tunnel, Close()).WillOnce(Return(0));

    ASSERT_TRUE(context->ConnectToSecureTunnel());
}

TEST_F(TestSecureTunnelContext, OnConnectionShutdown)
{
    /**
     * Creates onConnectionShutdown Lambda to set promise on invoke
     * Creates MockSecureTunnelingContext with onConnectionShutdown lambda
     * Invokes onConnectionShutdown
     * Verifies onConnectionShutdown invocation via promise
     */
    std::promise<void> promise;
    onConnectionShutdown = [&](SecureTunnelingContext *) -> void { promise.set_value(); };
    context = unique_ptr<MockSecureTunnelingContext>(
        new MockSecureTunnelingContext(manager, rootCa, accessToken, endpoint, port, onConnectionShutdown));

    EXPECT_CALL(*context, CreateSecureTunnel(_, _, _, _, _, _, _)).WillOnce(DoAll(InvokeArgument<1>(), Return(tunnel)));
    EXPECT_CALL(*tunnel, Connect()).WillOnce(Return(0));
    EXPECT_CALL(*tunnel, Close()).WillOnce(Return(0));

    ASSERT_TRUE(context->ConnectToSecureTunnel());

    EXPECT_EQ(std::future_status::ready, promise.get_future().wait_for(std::chrono::seconds(3)));
}
