// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef AWS_IOT_DEVICE_CLIENT_SAMPLESHADOW_H
#define AWS_IOT_DEVICE_CLIENT_SAMPLESHADOW_H

#include "../ClientBaseNotifier.h"
#include "../Feature.h"
#include "../SharedCrtResourceManager.h"
#include "../config/Config.h"
#include "../util/FileUtils.h"
#include <aws/iotshadow/IotShadowClient.h>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Shadow
            {
                class SampleShadowFeature : public Feature
                {
                  public:
                    static constexpr char NAME[] = "SampleShadow";

                    int init(
                        std::shared_ptr<SharedCrtResourceManager> manager,
                        std::shared_ptr<ClientBaseNotifier> notifier,
                        const PlainConfig &config);

                    // Interface methods defined in Feature.h
                    std::string getName() override;

                    int start() override;

                    int stop() override;

                  private:
                    /**
                     * \brief the ThingName to use
                     */
                    std::string thingName;
                    static constexpr char TAG[] = "SampleShadowFeature.cpp";

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
                     * \brief Whether the DeviceClient base has requested this feature to stop
                     */
                    std::atomic<bool> needStop{false};
                    /**
                     * \brief Name of shadow
                     */
                    std::string shadowName;
                    /**
                     * \brief Location of file containing data to upload into shaodow
                     */
                    std::string inputFile;
                    /**
                     * \brief Location of file to write the latest shadow document to
                     */
                    std::string outputFile;
                    /**
                     * \brief Default name of shadow document file
                     */
                    static constexpr char DEFAULT_SAMPLE_SHADOW_DOCUMENT_FILE[] = "default-sample-shadow-document";
                    /**
                     * \brief The default value in seconds for which Device client will wait for promise variables to be
                     * initialized. These promise variables will be initialized in respective callback methods
                     */
                    static constexpr int DEFAULT_WAIT_TIME_SECONDS = 10;
                    /**
                     * \brief an IotShadowClient used to make calls to the AWS IoT Shadow service
                     */
                    std::unique_ptr<Aws::Iotshadow::IotShadowClient> shadowClient;
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
                     * \brief a promise variable to check if subscription request to UpdateNamedShadow Delta
                     * topic was executed
                     */
                    std::promise<bool> subscribeShadowUpdateDeltaPromise;
                    /**
                     * \brief a promise variable to check if subscription request to UpdateNamedShadow Document
                     * topic was executed
                     */
                    std::promise<bool> subscribeShadowUpdateEventPromise;
                    /**
                     * \brief Subscribe all pertinent shadow topic (update/delta; update/document; update/rejected;
                     * update/accepted)
                     */
                    bool subscribeToPertinentShadowTopics();
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
                     * \brief Executed if our request to UpdateNamedShadow is accepted
                     * The response received on the shadow/update/document topic will be writen to the output file
                     *
                     * @param response information about the latest shadow document
                     * @param ioError a non-zero error code indicates a problem
                     */
                    void updateNamedShadowEventHandler(Iotshadow::ShadowUpdatedEvent *shadowUpdatedEvent, int ioError)
                        const;
                    /**
                     * \brief Executed if our request to UpdateNamedShadow is accepted and the delta exists in current
                     * shadow Will do the shadow sync after receiving the message from update/shadow/delta topic so a
                     * request will be sent to update the reported value to match desired ones
                     *
                     * @param response information including only the desired attributes that differ between the desired
                     * and reported sections in current shadow state.
                     * @param ioError a non-zero error code indicates a problem
                     */
                    void updateNamedShadowDeltaHandler(
                        Iotshadow::ShadowDeltaUpdatedEvent *shadowDeltaUpdatedEvent,
                        int ioError);
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
                     * \brief Acknowledgement that IoT Core has received our request for subscription to
                     * UpdateNamedShadow Document
                     *
                     * @param ioError a non-zero code here indicates a problem.
                     */
                    void ackSubscribeToUpdateEvent(int ioError);
                    /**
                     * \brief Acknowledgement that IoT Core has received our request for subscription to
                     * UpdateNamedShadow Delta
                     *
                     * @param ioError a non-zero code here indicates a problem.
                     */
                    void ackSubscribeToUpdateDelta(int ioError);
                    /**
                     * \brief Acknowledgement that IoT Core has received our UpdateNamedShadow Request
                     *
                     * @param ioError a non-zero code here indicates a problem. Turn on logging in IoT Core
                     * and check CloudWatch for more insights on errors
                     */
                    void ackUpdateNamedShadowStatus(int ioError) const;
                    /**
                     * \brief A function used to read and publish input data file to shadow
                     * @return true if readAndUpdateShadowFromFile successfully
                     */
                    void readAndUpdateShadowFromFile();
                    /**
                     * \brief A file monitor to detect any changes related with input file and its parent directory
                     * Once the any data is modified in input file, the shadow will be updated to sync with data in
                     * input file
                     */
                    void runFileMonitor();
                };
            } // namespace Shadow
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // AWS_IOT_DEVICE_CLIENT_SAMPLESHADOW_H