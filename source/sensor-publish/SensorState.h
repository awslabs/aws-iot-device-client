// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_SENSOR_STATE_H
#define DEVICE_CLIENT_SENSOR_STATE_H

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace SensorPublish
            {
                /**
                 * \brief States from the Sensor state machine
                 */
                enum class SensorState
                {
                    NotConnected,
                    Connecting,
                    Connected
                };
            } // namespace SensorPublish
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_SENSOR_STATE_H
