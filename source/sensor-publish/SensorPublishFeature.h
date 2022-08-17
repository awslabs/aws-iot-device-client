// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_SENSORPUBLISHFEATURE_H
#define DEVICE_CLIENT_SENSORPUBLISHFEATURE_H

#include "../ClientBaseNotifier.h"
#include "../Feature.h"
#include "../SharedCrtResourceManager.h"
#include "../config/Config.h"
#include "Sensor.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace SensorPublish
            {
                /**
                 * \brief SensorPublish publishes sensor data captured on the device to AWS IoT via MQTT.
                 *
                 * SensorPublish is a container for the list of Sensor that are configured by customer.
                 * Each Sensor reads and publishes independently of other sensors.
                 *
                 * SensorPublish notifies all Sensor instances in the list to stop and start.
                 */
                class SensorPublishFeature : public Feature
                {
                  protected:
                    /**
                     * \brief Used by the logger to specify source of log messages.
                     */
                    static constexpr char TAG[] = "SensorPublishFeature.cpp";

                    /**
                     * \brief The resource manager used to manage CRT resources
                     */
                    std::shared_ptr<SharedCrtResourceManager> mResourceManager;

                    /**
                     * \brief An interface used to notify the Client base if there is an event that requires its
                     * attention
                     */
                    std::shared_ptr<ClientBaseNotifier> mBaseNotifier;

                    /**
                     * \brief List of sensors
                     */
                    std::vector<std::unique_ptr<Sensor>> mSensors;

                    /**
                     * \brief createSensor is a factory function for sensors
                     */
                    virtual std::unique_ptr<Sensor> createSensor(
                        const PlainConfig::SensorPublish::SensorSettings &settings,
                        aws_allocator *allocator,
                        std::shared_ptr<Crt::Mqtt::MqttConnection> connection,
                        aws_event_loop *eventLoop) const;

                  public:
                    static constexpr char NAME[] = "Sensor Publish";

                    /**
                     * \brief Constructor
                     */
                    SensorPublishFeature() = default;

                    // Non-copyable.
                    SensorPublishFeature(const SensorPublishFeature &) = delete;
                    SensorPublishFeature &operator=(const SensorPublishFeature &) = delete;

                    /**
                     * \brief Initialize the SensorPublish feature.
                     *
                     * @param manager the shared resource manager
                     * @param notifier an ClientBaseNotifier used for notifying the client base of events or errors
                     * @param config passed in by the user via either the command line or configuration file
                     *
                     * @return a non-zero return code indicates a problem. The logs can be checked for more info
                     */
                    int init(
                        std::shared_ptr<SharedCrtResourceManager> manager,
                        std::shared_ptr<ClientBaseNotifier> notifier,
                        const PlainConfig &config);

                    /**
                     * \brief Start the feature
                     *
                     * @return an integer representing the SUCCESS or FAILURE of the start() operation
                     */
                    int start() override;

                    /**
                     * \brief Stop the feature
                     *
                     * @return an integer representing the SUCCESS or FAILURE of the stop() operation
                     */
                    int stop() override;

                    /**
                     * \brief For a given feature, returns its name
                     *
                     * @return a string value representing the feature's name
                     */
                    std::string getName() override;

                    /**
                     * \brief Returns the number of initialized sensors
                     */
                    std::size_t getSensorsSize() const;
                };
            } // namespace SensorPublish
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_SENSORPUBLISHFEATURE_H
