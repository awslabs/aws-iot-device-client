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
    const Aws::Iotsecuretunneling::OnConnectionComplete &onConnectionComplete,
    const Aws::Iotsecuretunneling::OnConnectionShutdown &onConnectionShutdown,
    const Aws::Iotsecuretunneling::OnSendDataComplete &onSendDataComplete,
    const Aws::Iotsecuretunneling::OnDataReceive &onDataReceive,
    const Aws::Iotsecuretunneling::OnStreamStart &onStreamStart,
    const Aws::Iotsecuretunneling::OnStreamReset &onStreamReset,
    const Aws::Iotsecuretunneling::OnSessionReset &onSessionReset)
    : secureTunnel(new Aws::Iotsecuretunneling::SecureTunnel(
          allocator,
          bootstrap,
          socketOptions,
          accessToken,
          localProxyMode,
          endpoint,
          rootCa,
          onConnectionComplete,
          onConnectionShutdown,
          onSendDataComplete,
          onDataReceive,
          onStreamStart,
          onStreamReset,
          onSessionReset))
{
}

SecureTunnelWrapper::SecureTunnelWrapper(
    Aws::Crt::Allocator *allocator,
    Aws::Crt::Io::ClientBootstrap *bootstrap,
    const Aws::Crt::Io::SocketOptions &socketOptions,
    const Aws::Crt::Http::HttpClientConnectionProxyOptions &proxyOptions,
    const std::string &accessToken,
    aws_secure_tunneling_local_proxy_mode localProxyMode,
    const std::string &endpoint,
    const std::string &rootCa,
    const Aws::Iotsecuretunneling::OnConnectionComplete &onConnectionComplete,
    const Aws::Iotsecuretunneling::OnConnectionShutdown &onConnectionShutdown,
    const Aws::Iotsecuretunneling::OnSendDataComplete &onSendDataComplete,
    const Aws::Iotsecuretunneling::OnDataReceive &onDataReceive,
    const Aws::Iotsecuretunneling::OnStreamStart &onStreamStart,
    const Aws::Iotsecuretunneling::OnStreamReset &onStreamReset,
    const Aws::Iotsecuretunneling::OnSessionReset &onSessionReset)
    : secureTunnel((Aws::Iotsecuretunneling::SecureTunnelBuilder(
                        allocator,
                        *bootstrap,
                        socketOptions,
                        accessToken,
                        localProxyMode,
                        endpoint))
                       .WithHttpClientConnectionProxyOptions(proxyOptions)
                       .WithRootCa(rootCa)
                       .WithOnConnectionComplete(onConnectionComplete)
                       .WithOnConnectionShutdown(onConnectionShutdown)
                       .WithOnSendDataComplete(onSendDataComplete)
                       .WithOnDataReceive(onDataReceive)
                       .WithOnStreamStart(onStreamStart)
                       .WithOnStreamReset(onSessionReset)
                       .WithOnSessionReset(onSessionReset)
                       .Build())
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
