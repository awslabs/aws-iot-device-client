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
                /**
                 * \brief Constructor
                 * @param connection An MqttConnection
                 */
                IotSecureTunnelingClientWrapper::IotSecureTunnelingClientWrapper(
                    std::shared_ptr<Aws::Crt::Mqtt::MqttConnection> connection)
                    : iotSecureTunnelingClient(
                          std::unique_ptr<IotSecureTunnelingClient>(new IotSecureTunnelingClient(connection)))
                {
                }

                /**
                 * \brief Subscribe to Mqtt Tunnel Notification Topic
                 * @param request The request object
                 * @param qos Mqtt Quality of Service
                 * @param handler Response handler callback
                 * @param onSubAck SubAck handler
                 */
                void IotSecureTunnelingClientWrapper::SubscribeToTunnelsNotify(
                    const Aws::Iotsecuretunneling::SubscribeToTunnelsNotifyRequest &request,
                    Aws::Crt::Mqtt::QOS qos,
                    const Iotsecuretunneling::OnSubscribeToTunnelsNotifyResponse &handler,
                    const Iotsecuretunneling::OnSubscribeComplete &onSubAck)
                {
                    iotSecureTunnelingClient->SubscribeToTunnelsNotify(
                        request, AWS_MQTT_QOS_AT_LEAST_ONCE, handler, onSubAck);
                }
            } // namespace SecureTunneling
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws
