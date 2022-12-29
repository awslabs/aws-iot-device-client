// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_DEVICEDEFENDERFEATURE_H
#define DEVICE_CLIENT_DEVICEDEFENDERFEATURE_H

#include <aws/iot/MqttClient.h>
#include <aws/iotdevicecommon/IotDevice.h>
#include <aws/iotdevicedefender/DeviceDefender.h>

#include "../ClientBaseNotifier.h"
#include "../Feature.h"
#include "../SharedCrtResourceManager.h"
#include "ReportTaskWrapper.h"

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace DeviceDefender
            {
                /**
                 * \brief Provides IoT Device Defender related functionality within the Device Client
                 */
                class DeviceDefenderFeature : public Feature
                {
                  public:
                    static constexpr char NAME[] = "Device Defender";
                    /**
                     * \brief Constructor
                     */
                    DeviceDefenderFeature();

                    /**
                     * \brief Initializes the Device Defender feature with all the required setup information, event
                     * handlers, and the shared MqttConnection
                     *
                     * @param manager the shared MqttConnectionManager
                     * @param notifier an ClientBaseNotifier used for notifying the client base of events or errors
                     * @param config configuration information passed in by the user via either the command line or
                     * configuration file
                     * @return a non-zero return code indicates a problem. The logs can be checked for more info
                     */
                    int init(
                        std::shared_ptr<SharedCrtResourceManager> manager,
                        std::shared_ptr<ClientBaseNotifier> notifier,
                        const PlainConfig &config);
                    void LoadFromConfig(const PlainConfig &config);

                    // Interface methods defined in Feature.h
                    std::string getName() override;
                    int start() override;
                    int stop() override;

                  protected:
                    /**
                     * \brief An interval in seconds used to determine how often to publish reports
                     */
                    int interval;
                    /**
                     * \brief the ThingName to use
                     */
                    std::string thingName;

                  private:
                    /**
                     * \brief Used by the logger to specify that log messages are coming from the Device Defender
                     * feature
                     */
                    static constexpr char TAG[] = "DeviceDefender.cpp";
                    /**
                     * \brief The resource manager used to manage CRT resources
                     */
                    std::shared_ptr<SharedCrtResourceManager> resourceManager;
                    /**
                     * \brief An interface used to notify the Client base if there is an event that requires its
                     * attention
                     */
                    std::shared_ptr<ClientBaseNotifier> baseNotifier;

                    /**
                     * \brief The first part of the MQTT topic that is built around the thingName,
                     * $aws/things/<thingName>/defender/metrics/json
                     */
                    static constexpr char TOPIC_PRE[] = "$aws/things/";
                    /**
                     * \brief The second part of the MQTT topic that is built around the thingName,
                     * $aws/things/<thingName>/defender/metrics/json
                     */
                    static constexpr char TOPIC_POST[] = "/defender/metrics/json";
                    /**
                     * \brief The third part of the MQTT topic that is built around the thingName
                     * published to by the service when reports are accepted.
                     * $aws/things/<thingName>/defender/metrics/json/accepted
                     */
                    static constexpr char TOPIC_ACCEPTED[] = "/accepted";
                    /**
                     * \brief The third part of the MQTT topic that is built around the thingName
                     * published to by the service when reports are rejected
                     * $aws/things/<thingName>/defender/metrics/json/rejected
                     */
                    static constexpr char TOPIC_REJECTED[] = "/rejected";

                    /**
                     * \brief The format of the Topic Filter
                     */
                    static constexpr char TOPIC_FORMAT[] = "%s%s%s%s";

                    /**
                     * \brief Factory method for ReportTask to facilitate mocking
                     */
                    virtual std::shared_ptr<AbstractReportTask> createReportTask();

                    /**
                     * \brief The Iot Device Defender SDK task responsible for publishing the reports
                     */
                    std::shared_ptr<AbstractReportTask> task;

                    /**
                     * \brief Called by feature start, this will build the task, add it to the eventLoopGroup in
                     * the SharedCrtResourceManager, & will start the task.  This function will also subscribe
                     * to the accepted/rejected Device Defender MQTT topics
                     */
                    void startDeviceDefender();
                    /**
                     * \brief Called when Iot Device Defender SDK task stops, this function will stop the Iot
                     * Device Defender SDK task, & unsubscribe from the accepted/rejected Device Defender MQTT topics
                     */
                    void stopDeviceDefender();

                    /**
                     * \brief Subscribes to Topic Filter Accepted/Rejected
                     */
                    virtual void subscribeToTopicFilter();

                    /**
                     * \Brief Unsubscribes to Topic Filter Accepted/Rejected
                     */
                    virtual void unsubscribeToTopicFilter();
                };
            } // namespace DeviceDefender
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_DEVICEDEFENDERFEATURE_H
