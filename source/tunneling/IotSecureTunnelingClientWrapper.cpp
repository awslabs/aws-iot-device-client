// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "IotSecureTunnelingClientWrapper.h"
#include "SecureTunnelingFeature.h"

using namespace Aws::Iotsecuretunneling;

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace SecureTunneling
            {
                IotSecureTunnelingClientWrapper::IotSecureTunnelingClientWrapper(
                    std::shared_ptr<Aws::Crt::Mqtt::MqttConnection> connection)
                {
                    iotSecureTunnelingClient = std::unique_ptr<IotSecureTunnelingClient>(new IotSecureTunnelingClient(connection));
                }
                void IotSecureTunnelingClientWrapper::SubscribeToTunnelsNotify(
                    const Aws::Iotsecuretunneling::SubscribeToTunnelsNotifyRequest &request,
                    Aws::Crt::Mqtt::QOS qos,
                    const Iotsecuretunneling::OnSubscribeToTunnelsNotifyResponse &handler,
                    const Iotsecuretunneling::OnSubscribeComplete &onSubAck)
                {
                    iotSecureTunnelingClient->SubscribeToTunnelsNotify(
                        request,
                        AWS_MQTT_QOS_AT_LEAST_ONCE,
                        handler,
                        onSubAck);
                }
            }
        }
    }
}


