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
    Aws::Iotsecuretunneling::OnConnectionComplete onConnectionComplete,
    Aws::Iotsecuretunneling::OnConnectionShutdown onConnectionShutdown,
    Aws::Iotsecuretunneling::OnSendDataComplete onSendDataComplete,
    Aws::Iotsecuretunneling::OnDataReceive onDataReceive,
    Aws::Iotsecuretunneling::OnStreamStart onStreamStart,
    Aws::Iotsecuretunneling::OnStreamReset onStreamReset,
    Aws::Iotsecuretunneling::OnSessionReset onSessionReset)
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
int SecureTunnelWrapper::Connect()
{
    if (secureTunnel)
    {
        return secureTunnel->Connect();
    }
    return AWS_OP_ERR;
}

int SecureTunnelWrapper::Close()
{
    if (secureTunnel)
    {
        return secureTunnel->Close();
    }
    return 0;
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
