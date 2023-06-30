// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "SecureTunnelWrapper.h"

using namespace Aws;
using namespace Aws::Iot;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::SecureTunneling;

SecureTunnelWrapper::SecureTunnelWrapper(
    Aws::Crt::Allocator *allocator,
    Aws::Crt::Io::ClientBootstrap *bootstrap,
    const Aws::Crt::Io::SocketOptions &socketOptions,
    const std::string &accessToken,
    aws_secure_tunneling_local_proxy_mode localProxyMode,
    const std::string &endpoint,
    const std::string &rootCa,
    const Aws::Iotsecuretunneling::OnConnectionSuccess &OnConnectionSuccess,
    const Aws::Iotsecuretunneling::OnConnectionFailure &OnConnectionFailure,
    const Aws::Iotsecuretunneling::OnConnectionShutdown &onConnectionShutdown,
    const Aws::Iotsecuretunneling::OnSendDataComplete &onSendDataComplete,
    const Aws::Iotsecuretunneling::OnDataReceive &onDataReceive,
    const Aws::Iotsecuretunneling::OnStreamStart &onStreamStart,
    const Aws::Iotsecuretunneling::OnStreamReset &onStreamReset,
    const Aws::Iotsecuretunneling::OnSessionReset &onSessionReset)
    : secureTunnel(new Aws::Iotsecuretunneling::SecureTunnelBuilder(
          allocator,
          socketOptions,
          accessToken,
          localProxyMode,
          endpoint).WithRootCa(rootCa)
                   .WithOnConnectionSuccess(OnConnectionSuccess)
                   .WithOnConnectionFailure(OnConnectionFailure)
                   .WithOnConnectionShutdown(onConnectionShutdown)
                   .WithOnSendDataComplete(onSendDataComplete)
                   .WithOnMessageReceived(onDataReceive)
                   .WithOnStreamStarted(onStreamStart)
                   .WithOnStreamReset(onStreamReset)
                   .WithOnSessionReset(onSessionReset)
                   .Build())


//          rootCa,
//      onConnectionComplete,
//      onConnectionShutdown,
//      onSendDataComplete,
//      onDataReceive,
//      onStreamStart,
//      onStreamReset,
//      onSessionReset
{
}
int SecureTunnelWrapper::Connect()
{
    return secureTunnel->Connect();
}

int SecureTunnelWrapper::Close()
{
    return secureTunnel->Close();
}

int SecureTunnelWrapper::SendData(const Aws::Crt::ByteCursor &data)
{
    return secureTunnel->SendData(data);
}

void SecureTunnelWrapper::Shutdown()
{
    secureTunnel->Shutdown();
}
bool SecureTunnelWrapper::IsValid()
{
    return secureTunnel->IsValid();
}
