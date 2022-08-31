// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef AWS_IOT_DEVICE_CLIENT_CONFIGSHADOW_H
#define AWS_IOT_DEVICE_CLIENT_CONFIGSHADOW_H

#include "../ClientBaseNotifier.h"
#include "../Feature.h"
#include "../SharedCrtResourceManager.h"
#include "../config/Config.h"
#include "../util/FileUtils.h"
#include <aws/crt/JsonObject.h>
#include <aws/crt/Optional.h>
#include <aws/iotshadow/IotShadowClient.h>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Shadow
            {
                class ConfigShadow
                {
                  public:
                    /**
                     * \brief Updating Device Client Configuration using Shadow feature
                     *
                     * @param MqttConnectionManager
                     * @param Device Client configuration passed in by the user via either the command line or
                     * configuration file
                     *
                     */
                    void reconfigureWithConfigShadow(
                        std::shared_ptr<SharedCrtResourceManager> resourceManager,
                        PlainConfig &config);
                    /**
                     * \brief Updates the local device client configuration with delta information in the config shadow
                     *
                     * @param config device client local configuration
                     */
                    void resetClientConfigWithJSON(
                        PlainConfig &config,
                        Crt::JsonView &deltaView,
                        Crt::JsonView &desiredView) const;

                  private:
                    static constexpr char TAG[] = "ConfigShadow.cpp";
                    /**
                     * \brief the config shadow name to use
                     */
                    static constexpr char DEFAULT_CONFIG_SHADOW_NAME[] = "DeviceClientConfigShadow";
                    /**
                     * \brief the ThingName to use
                     */
                    std::string thingName;
                    /**
                     * \brief Allow us to store the delta information in the config shadow from cloud
                     */
                    Aws::Crt::Optional<Aws::Crt::JsonObject> configDelta;
                    /**
                     * \brief Allow us to store the desired information in the config shadow from cloud
                     */
                    Aws::Crt::Optional<Aws::Crt::JsonObject> desiredConfig;
                    /**
                     * \brief The default value in seconds for which Device client will wait for promise variables to be
                     * initialized. These promise variables will be initialized in respective callback methods
                     */
                    static constexpr int DEFAULT_WAIT_TIME_SECONDS = 10;
                    /**
                     * \brief a promise variable to check if the config shadow exists in the cloud
                     */
                    std::promise<bool> configShadowExistsPromise;
                    /**
                     * \brief a promise variable to check if publish request for GetNamedShadow was received
                     */
                    std::promise<bool> shadowGetCompletedPromise;
                    /**
                     * \brief a promise variable to check if subscription request to GetNamedShadow Accepted
                     * topic was executed
                     */
                    std::promise<bool> subscribeShadowGetAcceptedPromise;
                    /**
                     * \brief a promise variable to check if subscription request to GetNamedShadow Rejected
                     * topic was executed
                     */
                    std::promise<bool> subscribeShadowGetRejectedPromise;
                    /**
                     * \brief a promise variable to check if publish request for UpdateNamedShadow was received
                     */
                    std::promise<bool> shadowUpdateCompletedPromise;
                    /**
                     * \brief a promise variable to check if subscription request to UpdateNamedShadow Accept
                     * topic was executed
                     */
                    std::promise<bool> subscribeShadowUpdateAcceptedPromise;
                    /**
                     * \brief a promise variable to check if subscription request to UpdateNamedShadow Rejected
                     * topic was executed
                     */
                    std::promise<bool> subscribeShadowUpdateRejectedPromise;
                    /**
                     * \brief fetches the information in config shadow from cloud by publishing a request to
                     * GetNamedShadow
                     * @param IotShadowClient used for subscribing and publishing request
                     * @return return true if publishing request completed and received corresponding response from
                     * cloud successfully
                     */
                    bool fetchRemoteConfigShadow(Iotshadow::IotShadowClient IotShadowClient);
                    /**
                     * A handler function called by the CRT SDK when our request to
                     * get a named shadow is accepted
                     * @param response information used to check if config shadow and config delta exists
                     * @param ioError a non-zero error code indicates a problem
                     */
                    void getNamedShadowAcceptedHandler(Iotshadow::GetShadowResponse *response, int ioError);
                    /**
                     * A handler function called by the CRT SDK when our request to
                     * get a named shadow is rejected
                     * @param response information about why the request is rejected
                     * @param ioError a non-zero error code indicates a problem
                     */
                    void getNamedShadowRejectedHandler(Iotshadow::ErrorResponse *errorResponse, int ioError);
                    /**
                     * \brief Executed if our request to UpdateNamedShadow is accepted
                     *
                     * @param response information about the updated shadow state
                     * @param ioError a non-zero error code indicates a problem
                     */
                    void updateNamedShadowAcceptedHandler(Iotshadow::UpdateShadowResponse *response, int ioError) const;
                    /**
                     * \brief Executed if our request to UpdateNamedShadow is rejected
                     *
                     * @param rejectedError information about the rejection
                     * @param ioError a non-zero error code indicates a problem
                     */
                    void updateNamedShadowRejectedHandler(Iotshadow::ErrorResponse *errorResponse, int ioError) const;
                    /**
                     * \brief Acknowledgement that IoT Core has received our request for subscription to
                     * UpdateNamedShadow Accepted
                     *
                     * @param ioError a non-zero code here indicates a problem.
                     */
                    void ackSubscribeToUpdateNamedShadowAccepted(int ioError);
                    /**
                     * \brief Acknowledgement that IoT Core has received our request for subscription to
                     * UpdateNamedShadow Rejected
                     *
                     * @param ioError a non-zero code here indicates a problem.
                     */
                    void ackSubscribeToUpdateNamedShadowRejected(int ioError);
                    /**
                     * \brief Acknowledgement that IoT Core has received our request for subscription to GetNamedShadow
                     * Accepted
                     *
                     * @param ioError a non-zero code here indicates a problem.
                     */
                    void ackSubscribeToGetNamedShadowAccepted(int ioError);
                    /**
                     * \brief Acknowledgement that IoT Core has received our request for subscription to GetNamedShadow
                     * Rejected
                     *
                     * @param ioError a non-zero code here indicates a problem.
                     */
                    void ackSubscribeToGetNamedShadowRejected(int ioError);
                    /**
                     * \brief Acknowledgement that IoT Core has received our GetNamedShadow Request
                     *
                     * @param ioError a non-zero code here indicates a problem. Turn on logging in IoT Core
                     * and check CloudWatch for more insights on errors
                     */
                    void ackGetNamedShadowStatus(int ioError);
                    /**
                     * \brief Acknowledgement that IoT Core has received our UpdateNamedShadow Request
                     *
                     * @param ioError a non-zero code here indicates a problem. Turn on logging in IoT Core
                     * and check CloudWatch for more insights on errors
                     */
                    void ackUpdateNamedShadowStatus(int ioError);
                    /**
                     * \brief Subscribes to pertinent named shadow GET and UPDATE topics
                     *
                     * @param ioError a non-zero code here indicates a problem. Turn on logging in IoT Core
                     * and check CloudWatch for more insights on errors
                     * @return true if all topics are subscribed successfully
                     */
                    bool subscribeGetAndUpdateNamedShadowTopics(Iotshadow::IotShadowClient iotShadowClient);
                    /**
                     * \brief Sends the request to update config shadow with the latest features configuration in device
                     * client
                     *
                     * @param iotShadowClient
                     * @param config device client local configuration
                     */
                    void updateShadowWithLocalConfig(Iotshadow::IotShadowClient iotShadowClient, PlainConfig &config);
                    /**
                     * \brief Loads local configuration into a json object
                     * @param config device client local configuration
                     * @param jsonObj contains all device client features' configuration
                     */
                    void loadFeatureConfigIntoJsonObject(PlainConfig &config, Aws::Crt::JsonObject &jsonObj) const;
                };
            } // namespace Shadow
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // AWS_IOT_DEVICE_CLIENT_CONFIGSHADOW_H