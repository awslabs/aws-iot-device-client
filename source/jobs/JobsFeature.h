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

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Jobs
            {
                /**
                 * \brief Provides IoT Jobs related functionality within the Device Client
                 */
                class JobsFeature : public Feature
                {
                  private:
                    /**
                     * \brief Used by the logger to specify that log messages are coming from the Jobs feature
                     */
                    const char *TAG = "JobsFeature.cpp";
                    /**
                     * \brief A field within a Job document that the Jobs feature uses to determine which
                     * job to run
                     */
                    const char *JOB_ATTR_OP = "operation";
                    /**
                     * \brief A field within a Job document that the Jobs feature uses to determine which
                     * arguments to pass to the operation being run
                     */
                    const char *JOB_ATTR_ARGS = "args";
                    /**
                     * \brief A field within a Job document that the Jobs feature uses to determine whether
                     * STDERR output issued by the child process should be marked as a job failure in IoT Core
                     */
                    const char *JOB_ATTR_ALLOW_STDERR = "allowStdErr";
                    /**
                     * \brief A field within a Job document that the Jobs feature uses to determine the location of
                     * the executable that should be run to handle this job
                     *
                     * If the path attribute is specified as "default", the Jobs feature will attempt to find the
                     * executable in the path configured via the configuration file or command line arguments.
                     * Otherwise, the Jobs feature will assume that the $PATH contains the executable
                     */
                    const char *JOB_ATTR_PATH = "path";
                    /**
                     * \brief A keyword that can be specified as the "path" in a job doc to tell the Jobs feature to
                     * use the configured handler directory when looking for an executable matching the specified
                     * operation
                     */
                    const char *DEFAULT_PATH_KEYWORD = "default";
                    /**
                     * \brief The default directory that the Jobs feature will use to find executables matching
                     * an incoming job document's operation attribute
                     */
                    /**
                     * \brief A limit enforced by the AWS IoT Jobs API on the maximum number of characters allowed
                     * to be provided in a StatusDetail entry when calling the UpdateJobExecution API
                     */
                    const size_t MAX_STATUS_DETAIL_LENGTH = 1024;
                    const int UPDATE_JOB_EXECUTION_REJECTED_CODE = -1;
                    const std::string DEFAULT_JOBS_HANDLER_DIR = "/etc/aws-iot-device-client/handlers/";

                    /**
                     * \brief Whether the DeviceClient base has requested this feature to stop
                     */
                    bool needStop = false;
                    /**
                     * \brief Whether the jobs feature is currently executing a job
                     */
                    bool handlingJob = false;
                    /**
                     * \brief A mutex used to control access to the stop and handling job flags
                     */
                    std::mutex canStopLock;

                    /**
                     * \brief A lock used to control access to the map of EphemeralPromise
                     */
                    std::mutex updateJobExecutionPromisesLock;
                    /**
                     * \brief Allows us to map UpdateJobExecution responses back to their original request
                     */
                    Aws::Crt::Map<Aws::Crt::String, Aws::Iot::DeviceClient::Jobs::EphemeralPromise<int>>
                        updateJobExecutionPromises;
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
                     * \brief an IotJobsClient used to make calls to the AWS IoT Jobs service
                     */
                    std::unique_ptr<Aws::Iotjobs::IotJobsClient> jobsClient;

                    /**
                     * \brief the ThingName to use
                     */
                    std::string thingName;
                    /**
                     * \brief User provided handler directory passed either through command-line arguments or through
                     * the Json configuration file
                     */
                    std::string jobHandlerDir = DEFAULT_JOBS_HANDLER_DIR;
                    /**
                     * \brief whether to include StdOut information when calling UpdateJobExecution
                     *
                     * By default, we do not include the STDOUT output from the child process when updating the job
                     * execution, since this output can be lengthy. TODO we should allow customers to pass this
                     * attribute as part of the job doc in case the STDOUT is useful in debugging.
                     */
                    bool includeStdoutInUpdates = false;

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

                    // Outgoing Mqtt messages/topic subscriptions
                    /**
                     * \brief Publishes a request to startNextPendingJobExecution
                     */
                    void publishStartNextPendingJobExecutionRequest();

                    /**
                     * \brief Wrapper struct to aggregate JobEngine output for updating a job execution status
                     */
                    struct JobExecutionStatusInfo
                    {
                        Aws::Iotjobs::JobStatus status;
                        std::string reason;
                        std::string stdoutput;
                        std::string stderror;
                    };

                    /**
                     * \brief Attempts to update a job execution to the provided status
                     * @param data JobExecutionData containing information about the job
                     * @param statusInfo status information including the Job status, as well as STDOUT and STDERR from
                     * the child process
                     */
                    void publishUpdateJobExecutionStatus(
                        Aws::Iotjobs::JobExecutionData data,
                        JobExecutionStatusInfo statusInfo);
                    /**
                     * \brief Creates a subscription to the startNextPendingJobExecution topic
                     */
                    void subscribeToStartNextPendingJobExecution();
                    /**
                     * \brief Creates a subscription to NextJobChangedEvents
                     */
                    void subscribeToNextJobChangedEvents();
                    /**
                     * \brief Enables the Jobs feature to receive response information from the IoT Jobs service when an
                     * update is accepted
                     * @param jobId the job ID to listen for. Use "+" to subscribe for all job executions for this
                     * thing.
                     */
                    void subscribeToUpdateJobExecutionStatusAccepted(std::string jobId);
                    /**
                     * \brief Enables the Jobs feature to receive response information from the IoT Jobs service when an
                     * update is rejected
                     * @param jobId the job ID to listen for. Use "+" to subscribe for all job executions for this
                     * thing.
                     */
                    void subscribeToUpdateJobExecutionStatusRejected(std::string jobId);

                    // Incoming Mqtt message handlers
                    /**
                     * \brief Executed upon receiving a response to our request to StartNextPendingJob
                     *
                     * @param response the response from Iot Core
                     * @param ioError a non-zero error code indicates a problem
                     */
                    void startNextPendingJobReceivedHandler(
                        Iotjobs::StartNextJobExecutionResponse *response,
                        int ioError);
                    /**
                     * \brief Executed if our request to StartNextPendingJob is rejected
                     *
                     * @param rejectedError information about the rejection
                     * @param ioError a non-zero error code indicates a problem
                     */
                    void startNextPendingJobRejectedHandler(Iotjobs::RejectedError *rejectedError, int ioError);
                    /**
                     * \brief Executed when the next job for this thing to execute has changed.
                     *
                     * This typically happens after updating a job execution and the Jobs API is notifying the Device
                     * Client that we have a new job to execute
                     * @param event information about the next job
                     * @param ioErr a non-zero error code indicates a problem
                     */
                    void nextJobChangedHandler(Iotjobs::NextJobExecutionChangedEvent *event, int ioErr);
                    /**
                     * A handler function called by the CRT SDK when we receive a response to our request to
                     * update a job execution status
                     * @param response information used to determine which job execution was updated
                     * @param ioError a non-zero error code indicates a problem
                     */
                    void updateJobExecutionStatusAcceptedHandler(
                        Iotjobs::UpdateJobExecutionResponse *response,
                        int ioError);
                    /**
                     * A handler function called by the CRT SDK when our request to
                     * update a job execution status is rejected
                     * @param response information used to determine which job execution was updated
                     * @param ioError a non-zero error code indicates a problem
                     */
                    void updateJobExecutionStatusRejectedHandler(Iotjobs::RejectedError *rejectedError, int ioError);

                    // Process a Job
                    /**
                     * \brief Called to begin the execution of a job on the device
                     *
                     * @param job the job to execute
                     */
                    void executeJob(Iotjobs::JobExecutionData job);
                    // Begin running the Jobs feature
                    /**
                     * \brief Begins running the Jobs feature
                     */
                    void runJobs();

                  public:
                    virtual std::string getName();
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
                        std::shared_ptr<SharedCrtResourceManager> manager,
                        std::shared_ptr<ClientBaseNotifier> notifier,
                        const PlainConfig &config);

                    // Interface methods defined in Feature.h
                    virtual int start();
                    virtual int stop();
                };
            } // namespace Jobs
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_JOBSFEATURE_H
