// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef AWS_IOT_DEVICE_CLIENT_SECURETUNNELWRAPPER_H
#define AWS_IOT_DEVICE_CLIENT_SECURETUNNELWRAPPER_H

#include <aws/iotsecuretunneling/SecureTunnel.h>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace SecureTunneling
            {
                class SecureTunnelWrapper
                {
                  public:
                    SecureTunnelWrapper() = default;
                    virtual ~SecureTunnelWrapper() = default;
                    SecureTunnelWrapper(
                        Aws::Crt::Allocator *allocator,
                        Aws::Crt::Io::ClientBootstrap *clientBootstrap,
                        const Aws::Crt::Io::SocketOptions &socketOptions,

                        const std::string &accessToken,
                        aws_secure_tunneling_local_proxy_mode localProxyMode,
                        const std::string &endpointHost,
                        const std::string &rootCa,

                        Aws::Iotsecuretunneling::OnConnectionComplete onConnectionComplete,
                        Aws::Iotsecuretunneling::OnConnectionShutdown onConnectionShutdown,
                        Aws::Iotsecuretunneling::OnSendDataComplete onSendDataComplete,
                        Aws::Iotsecuretunneling::OnDataReceive onDataReceive,
                        Aws::Iotsecuretunneling::OnStreamStart onStreamStart,
                        Aws::Iotsecuretunneling::OnStreamReset onStreamReset,
                        Aws::Iotsecuretunneling::OnSessionReset onSessionReset);

                    virtual int Connect();

                    virtual int Close();

                    virtual int SendData(const Aws::Crt::ByteCursor &data);

                    virtual void Shutdown();

                    virtual bool IsValid();

                    std::unique_ptr<Aws::Iotsecuretunneling::SecureTunnel> secureTunnel;

                  private:
                    /**
                     * \brief Used by the logger to specify that log messages are coming from this class
                     */
                    static constexpr char TAG[] = "SecureTunnelingContext.cpp";
                };
            } // namespace SecureTunneling
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // AWS_IOT_DEVICE_CLIENT_SECURETUNNELWRAPPER_H
