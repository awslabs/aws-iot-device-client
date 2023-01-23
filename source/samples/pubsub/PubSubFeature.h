// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_PUBSUBFEATURE_H
#define DEVICE_CLIENT_PUBSUBFEATURE_H

#include <aws/iot/MqttClient.h>

#include "../../ClientBaseNotifier.h"
#include "../../Feature.h"
#include "../../SharedCrtResourceManager.h"
#include "../../config/Config.h"
#include "../../util/FileUtils.h"

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Samples
            {
                /**
                 * \brief Provides IoT PubSub related sample functionality within the Device Client.
                 * When enabled The Pub Sub sample feature will publish data from a configured `publish-file` to a
                 * `publish-topic`, and will recieve messages on a configured `subscribe-topic` and write them to a
                 * `subscribe-file`. More information can be found in the `samples/pubsub/README.md`
                 */
                class PubSubFeature : public Feature
                {
                  public:
                    static constexpr char NAME[] = "Pub Sub Sample";
                    static constexpr char DEFAULT_PUBLISH_FILE[] = "~/.aws-iot-device-client/pubsub/publish-file.txt";
                    static constexpr char DEFAULT_SUBSCRIBE_FILE[] =
                        "~/.aws-iot-device-client/pubsub/subscribe-file.txt";
                    bool createPubSub(
                        const PlainConfig &config,
                        const std::string &absFilePath,
                        const aws_byte_buf *payload) const;
                    /**
                     * \brief Initializes the PubSub feature with all the required setup information, event
                     * handlers, and the SharedCrtResourceManager
                     *
                     * @param manager The resource manager used to manage CRT resources
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

                  private:
                    /**
                     * \brief the ThingName to use
                     */
                    std::string thingName;
                    static constexpr char TAG[] = "samples/PubSubFeature.cpp";

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
                     * \brief Whether the DeviceClient base has requested this feature to stop.
                     */
                    std::atomic<bool> needStop{false};
                    /**
                     * \brief Topic for publishing data to
                     */
                    std::string pubTopic;
                    /**
                     * \brief Location of file containing data to publish
                     */
                    std::string pubFile = DEFAULT_PUBLISH_FILE;
                    /**
                     * \brief Whether or not to start the inotify thread to republish changes.
                     */
                    bool publishOnChange = false;
                    /**
                     * \brief Topic to subscribe to
                     */
                    std::string subTopic;
                    /**
                     * \brief Topic to write subscription payloads to
                     */
                    std::string subFile = DEFAULT_SUBSCRIBE_FILE;
                    /**
                     * \brief Default payload if no publish file was provided
                     */
                    static const std::string DEFAULT_PUBLISH_PAYLOAD;
                    /**
                     * \brief Subscription payload used to retrigger the publish actions
                     */
                    static const std::string PUBLISH_TRIGGER_PAYLOAD;
                    /**
                     * \brief Workflow function for publishing data to the configured topic
                     */
                    void publishFileData();
                    /**
                     * \brief Called to read the publish file data
                     * @param buf Buffer to read data into
                     * @return 0 on success
                     */
                    int getPublishFileData(aws_byte_buf *buf) const;
                    /**
                     * \brief A file monitor to detect any changes related to the input file.
                     * Once any data is modified in the publish-file, the client will publish
                     * the data to the publish-topic.
                     */
                    void runFileMonitor();
                };
            } // namespace Samples
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_SAMPLESFEATURE_H
