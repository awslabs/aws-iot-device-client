// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef AWS_IOT_DEVICE_CLIENT_IOTSECURETUNNELINGCLIENTWRAPPER_H
#define AWS_IOT_DEVICE_CLIENT_IOTSECURETUNNELINGCLIENTWRAPPER_H

#include "aws/iotsecuretunneling/IotSecureTunnelingClient.h"

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace SecureTunneling
            {
                class AbstractIotSecureTunnelingClient
                {
                  public:
                    virtual ~AbstractIotSecureTunnelingClient() = default;
                    virtual void SubscribeToTunnelsNotify(
                        const Aws::Iotsecuretunneling::SubscribeToTunnelsNotifyRequest &request,
                        Aws::Crt::Mqtt::QOS qos,
                        const Iotsecuretunneling::OnSubscribeToTunnelsNotifyResponse &handler,
                        const Iotsecuretunneling::OnSubscribeComplete &onSubAck) = 0;
                };

                class IotSecureTunnelingClientWrapper : public AbstractIotSecureTunnelingClient
                {
                  public:
                    explicit IotSecureTunnelingClientWrapper(
                        std::shared_ptr<Aws::Crt::Mqtt::MqttConnection> connection);
                    virtual void SubscribeToTunnelsNotify(
                        const Aws::Iotsecuretunneling::SubscribeToTunnelsNotifyRequest &request,
                        Aws::Crt::Mqtt::QOS qos,
                        const Iotsecuretunneling::OnSubscribeToTunnelsNotifyResponse &handler,
                        const Iotsecuretunneling::OnSubscribeComplete &onSubAck) override;

                    std::unique_ptr<Iotsecuretunneling::IotSecureTunnelingClient> iotSecureTunnelingClient;
                };
            } // namespace SecureTunneling
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // AWS_IOT_DEVICE_CLIENT_IOTSECURETUNNELINGCLIENTWRAPPER_H
