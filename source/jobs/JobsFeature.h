// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_JOBSFEATURE_H
#define DEVICE_CLIENT_JOBSFEATURE_H

#include <aws/iot/MqttClient.h>
#include <aws/iotjobs/IotJobsClient.h>
#include <aws/iotjobs/JobExecutionData.h>

#include "../ClientBaseNotifier.h"
#include "../Feature.h"
#include "../SharedCrtResourceManager.h"
#include "EphemeralPromise.h"
#include "IotJobsClientWrapper.h"
#include "JobDocument.h"
#include "JobEngine.h"

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Jobs
            {
                class AbstractIotJobsClient;
                /**
                 * \brief Provides IoT Jobs related functionality within the Device Client
                 */
                class JobsFeature : public Feature
                {
                  public:
                    virtual std::string getName() override;

                    /**
                     * \brief Wrapper struct to aggregate JobEngine output for updating a job execution status
                     */

                    struct JobExecutionStatusInfo
                    {
                        Aws::Iotjobs::JobStatus status;
                        std::string reason;
                        std::string stdoutput;
                        std::string stderror;

                        explicit JobExecutionStatusInfo(Aws::Iotjobs::JobStatus status) : status(status) {}
                        JobExecutionStatusInfo(
                            Aws::Iotjobs::JobStatus status,
                            const std::string &reason,
                            const std::string &stdoutput,
                            const std::string &stderror)
                            : status(status), reason(reason), stdoutput(stdoutput), stderror(stderror)
                        {
                        }
                    };

                    /**
                     * \brief Initializes the Jobs feature with all the required setup information, event handlers, and
                     * the shared MqttConnection
                     *
                     * @param manager the shared MqttConnectionManager
                     * @param notifier an ClientBaseNotifier used for notifying the client base of events or errors
                     * @param config configuration information passed in by the user via either the command line or
                     * configuration file
                     * @return a non-zero return code indicates a problem. The logs can be checked for more info
                     */
                    virtual int init(
                        std::shared_ptr<Crt::Mqtt::MqttConnection> connection,
                        std::shared_ptr<ClientBaseNotifier> notifier,
                        const PlainConfig &config);

                    // Interface methods defined in Feature.h
                    virtual int start() override;
                    virtual int stop() override;

                  protected:
                    /**
                     * \brief Begins running the Jobs feature
                     */
                    void runJobs();
                    /**
                     * \brief An enum used for UpdateJobExecution responses
                     */
                    enum UpdateJobExecutionResponseType
                    {
                        ACCEPTED,
                        RETRYABLE_ERROR,
                        NON_RETRYABLE_ERROR
                    };

                  private:
                    /**
                     * \brief Used by the logger to specify that log messages are coming from the Jobs feature
                     */
                    const char *TAG = "JobsFeature.cpp";

                    /**
                     * \brief The default directory that the Jobs feature will use to find executables matching
                     * an incoming job document's operation attribute
                     */
                    static const std::string DEFAULT_JOBS_HANDLER_DIR;

                    /**
                     * \brief A limit enforced by the AWS IoT Jobs API on the maximum number of characters allowed
                     * to be provided in a StatusDetail entry when calling the UpdateJobExecution API
                     */
                    const size_t MAX_STATUS_DETAIL_LENGTH = 1024;

                    /**
                     * \brief Whether the DeviceClient base has requested this feature to stop
                     */
                    std::atomic<bool> needStop{false};
                    /**
                     * \brief Whether the jobs feature is currently executing a job
                     */
                    std::atomic<bool> handlingJob{false};

                    /**
                     * \brief A lock used to control access to the map of EphemeralPromise
                     */
                    std::mutex updateJobExecutionPromisesLock;

                    /**
                     * \brief Allows us to map UpdateJobExecution responses back to their original request
                     */
                    Aws::Crt::Map<
                        Aws::Crt::String,
                        Aws::Iot::DeviceClient::Jobs::EphemeralPromise<UpdateJobExecutionResponseType>>
                        updateJobExecutionPromises;

                    std::mutex latestJobsNotificationLock;
                    Aws::Iotjobs::JobExecutionData latestJobsNotification;

                    /**
                     * \brief Mqtt Connection for IotJobsClient
                     */
                    std::shared_ptr<Crt::Mqtt::MqttConnection> mqttConnection;
                    /**
                     * \brief An interface used to notify the Client base if there is an event that requires its
                     * attention
                     */
                    std::shared_ptr<ClientBaseNotifier> baseNotifier;

                    /**
                     * \brief an IotJobsClient used to make calls to the AWS IoT Jobs service
                     */
                    std::shared_ptr<AbstractIotJobsClient> jobsClient;

                    /**
                     * \brief the ThingName to use
                     */
                    std::string thingName;
                    /**
                     * \brief User provided handler directory passed either through command-line arguments or through
                     * the Json configuration file
                     */
                    std::string jobHandlerDir = DEFAULT_JOBS_HANDLER_DIR;

                    // Ack handlers
                    /**
                     * \brief Acknowledgement that IoT Core has received our request for subscription to NextJobChanged
                     *
                     * @param ioError a non-zero code here indicates a problem. Turn on logging in IoT Core
                     * and check CloudWatch for more insights on errors
                     */
                    void ackSubscribeToNextJobChanged(int ioError);
                    /**
                     * \brief Acknowledgement that IoT Core has received our request for subscription to
                     * StartNextJobAccepted
                     *
                     * @param ioError a non-zero code here indicates a problem. Turn on logging in IoT Core
                     * and check CloudWatch for more insights on errors
                     */
                    void ackSubscribeToStartNextJobAccepted(int ioError);
                    /**
                     * \brief Acknowledgement that IoT Core has received our request for subscription to
                     * StartNextJobRejected
                     *
                     * @param ioError a non-zero code here indicates a problem. Turn on logging in IoT Core
                     * and check CloudWatch for more insights on errors
                     */
                    void ackSubscribeToStartNextJobRejected(int ioError);
                    /**
                     * \brief Acknowledgement that IoT Core has received our StartNextPendingJob message
                     *
                     * @param ioError a non-zero code here indicates a problem. Turn on logging in IoT Core
                     * and check CloudWatch for more insights on errors
                     */
                    void ackStartNextPendingJobPub(int ioError);
                    /**
                     * \brief Acknowledgement that IoT Core has received our UpdateJobExecutionStatus message
                     *
                     * @param ioError a non-zero code here indicates a problem. Turn on logging in IoT Core
                     * and check CloudWatch for more insights on errors
                     */
                    void ackUpdateJobExecutionStatus(int ioError);
                    void ackSubscribeToUpdateJobExecutionAccepted(int ioError);
                    void ackSubscribeToUpdateJobExecutionRejected(int ioError);
                    std::promise<int> updateAcceptedPromise;
                    std::promise<int> updateRejectedPromise;

                    // Outgoing Mqtt messages/topic subscriptions
                    /**
                     * \brief Publishes a request to startNextPendingJobExecution
                     */
                    virtual void publishStartNextPendingJobExecutionRequest();

                    /**
                     * \brief Attempts to update a job execution to the provided status
                     * @param data JobExecutionData containing information about the job
                     * @param statusInfo status information including the Job status, as well as STDOUT and STDERR from
                     * the child process
                     */
                    void publishUpdateJobExecutionStatus(
                        Aws::Iotjobs::JobExecutionData data,
                        JobExecutionStatusInfo statusInfo,
                        std::function<void(void)> onCompleteCallback = nullptr);

                    virtual void publishUpdateJobExecutionStatusWithRetry(
                        Aws::Iotjobs::JobExecutionData data,
                        JobExecutionStatusInfo statusInfo,
                        Aws::Crt::Map<Aws::Crt::String, Aws::Crt::String> statusDetails,
                        std::function<void(void)> onCompleteCallback);
                    /**
                     * \brief Creates a subscription to the startNextPendingJobExecution topic
                     */
                    virtual void subscribeToStartNextPendingJobExecution();
                    /**
                     * \brief Creates a subscription to NextJobChangedEvents
                     */
                    virtual void subscribeToNextJobChangedEvents();
                    /**
                     * \brief Enables the Jobs feature to receive response information from the IoT Jobs service when an
                     * update is accepted
                     * @param jobId the job ID to listen for. Use "+" to subscribe for all job executions for this
                     * thing.
                     */
                    virtual void subscribeToUpdateJobExecutionStatusAccepted(const std::string &jobId);
                    /**
                     * \brief Enables the Jobs feature to receive response information from the IoT Jobs service when an
                     * update is rejected
                     * @param jobId the job ID to listen for. Use "+" to subscribe for all job executions for this
                     * thing.
                     */
                    virtual void subscribeToUpdateJobExecutionStatusRejected(const std::string &jobId);

                    // Incoming Mqtt message handlers
                    /**
                     * \brief Executed upon receiving a response to our request to StartNextPendingJob
                     *
                     * @param response the response from Iot Core
                     * @param ioError a non-zero error code indicates a problem
                     */
                    virtual void startNextPendingJobReceivedHandler(
                        Iotjobs::StartNextJobExecutionResponse *response,
                        int ioError);
                    /**
                     * \brief Executed if our request to StartNextPendingJob is rejected
                     *
                     * @param rejectedError information about the rejection
                     * @param ioError a non-zero error code indicates a problem
                     */
                    virtual void startNextPendingJobRejectedHandler(Iotjobs::RejectedError *rejectedError, int ioError);
                    /**
                     * \brief Executed when the next job for this thing to execute has changed.
                     *
                     * This typically happens after updating a job execution and the Jobs API is notifying the Device
                     * Client that we have a new job to execute
                     * @param event information about the next job
                     * @param ioErr a non-zero error code indicates a problem
                     */
                    virtual void nextJobChangedHandler(Iotjobs::NextJobExecutionChangedEvent *event, int ioErr);
                    /**
                     * A handler function called by the CRT SDK when we receive a response to our request to
                     * update a job execution status
                     * @param response information used to determine which job execution was updated
                     * @param ioError a non-zero error code indicates a problem
                     */
                    virtual void updateJobExecutionStatusAcceptedHandler(
                        Iotjobs::UpdateJobExecutionResponse *response,
                        int ioError);
                    /**
                     * A handler function called by the CRT SDK when our request to
                     * update a job execution status is rejected
                     * @param response information used to determine which job execution was updated
                     * @param ioError a non-zero error code indicates a problem
                     */
                    virtual void updateJobExecutionStatusRejectedHandler(
                        Iotjobs::RejectedError *rejectedError,
                        int ioError);

                    /**
                     * \brief Called to begin the execution of a job on the device
                     *
                     * @param job the job to execute
                     */
                    virtual void executeJob(const Iotjobs::JobExecutionData &job, const PlainJobDocument &jobDocument);

                    void initJob(const Iotjobs::JobExecutionData &job);

                    /**
                     * \brief Given a job notification, determines whether it's a duplicate message.
                     *
                     * This method was originally intended to handle scenarios such as network instability
                     * or loss where the jobs feature may receive multiple instances of the same message.
                     * This allows us to eliminate duplicates that would otherwise cause the Jobs feature
                     * to run the same job more than once.
                     * @param job
                     * @return true if it's a duplicate, false otherwise
                     */
                    bool isDuplicateNotification(Iotjobs::JobExecutionData job);

                    /**
                     * \brief Stores information about a job notification
                     *
                     * @param job
                     */
                    void copyJobsNotification(Iotjobs::JobExecutionData job);

                    /**
                     * \brief virtual functions to facilitate injecting mocks for testing
                     */
                    virtual std::shared_ptr<AbstractIotJobsClient> createJobsClient();

                    virtual std::shared_ptr<JobEngine> createJobEngine();
                };
            } // namespace Jobs
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_JOBSFEATURE_H
