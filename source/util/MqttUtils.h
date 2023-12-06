// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <cstddef>
#include <string>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Util
            {
                namespace MqttUtils
                {
                    // Limits based on documentation linked below and copy-pasted into comment.
                    //
                    // https://docs.aws.amazon.com/general/latest/gr/iot-core.html#message-broker-limits
                    //
                    // The topic passed to AWS IoT Core when sending a publish request can be no larger
                    // than 256 bytes of UTF-8 encoded characters. This excludes the first 3 mandatory
                    // segments for Basic Ingest topics ($AWS/rules/rule-name/).
                    //
                    static constexpr std::size_t MAX_LENGTH_OF_TOPIC{256};

                    /**
                     * \brief Returns true if topic is valid AWS Iot MQTT topic name
                     */
                    bool ValidateAwsIotMqttTopicName(std::string topic);
                } // namespace MqttUtils
            }     // namespace Util
        }         // namespace DeviceClient
    }             // namespace Iot
} // namespace Aws
