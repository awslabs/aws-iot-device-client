// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "JobsFeature.h"
#include "../logging/LoggerFactory.h"
#include "../util/FileUtils.h"
#include "../util/Retry.h"
#include "../util/UniqueString.h"
#include "EphemeralPromise.h"
#include "JobDocument.h"
#include "JobEngine.h"

#include <aws/iotjobs/NextJobExecutionChangedEvent.h>
#include <aws/iotjobs/NextJobExecutionChangedSubscriptionRequest.h>
#include <aws/iotjobs/RejectedError.h>
#include <aws/iotjobs/RejectedErrorCode.h>
#include <aws/iotjobs/StartNextJobExecutionResponse.h>
#include <aws/iotjobs/StartNextPendingJobExecutionRequest.h>
#include <aws/iotjobs/StartNextPendingJobExecutionSubscriptionRequest.h>
#include <aws/iotjobs/UpdateJobExecutionRequest.h>
#include <aws/iotjobs/UpdateJobExecutionResponse.h>
#include <aws/iotjobs/UpdateJobExecutionSubscriptionRequest.h>
#include <wordexp.h>

#include <thread>
#include <utility>

using namespace std;
using namespace Aws::Iot;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::Logging;
using namespace Aws::Iot::DeviceClient::Util;
using namespace Aws::Iot::DeviceClient::Jobs;
using namespace Aws::Iotjobs;

const std::string JobsFeature::DEFAULT_JOBS_HANDLER_DIR = "~/.aws-iot-device-client/jobs/";

string JobsFeature::getName()
{
    return string("Jobs");
}

void JobsFeature::ackSubscribeToNextJobChanged(int ioError)
{
    LOGM_DEBUG(TAG, "Ack received for SubscribeToNextJobChanged with code {%d}", ioError);
    if (ioError)
    {
        // TODO We need to implement a strategy for what do when our subscription fails
        string errorMessage =
            FormatMessage("Encountered ioError {%d} while attempting to subscribe to NextJobChanged", ioError);
        LOG_ERROR(TAG, errorMessage.c_str());
        baseNotifier->onError(this, ClientBaseErrorNotification::SUBSCRIPTION_FAILED, errorMessage);
    }
}

void JobsFeature::ackStartNextPendingJobPub(int ioError)
{
    LOGM_DEBUG(TAG, "Ack received for StartNextPendingJobPub with code {%d}", ioError);
}

void JobsFeature::ackSubscribeToStartNextJobAccepted(int ioError)
{
    LOGM_DEBUG(TAG, "Ack received for SubscribeToStartNextJobAccepted with code {%d}", ioError);
    if (ioError)
    {
        // TODO We need to implement a strategy for what do when our subscription fails
        string errorMessage = "Encountered an ioError while attempting to subscribe to StartNextJobAccepted";
        LOG_ERROR(TAG, errorMessage.c_str());
        baseNotifier->onError(this, ClientBaseErrorNotification::SUBSCRIPTION_FAILED, errorMessage);
    }
}

void JobsFeature::ackSubscribeToStartNextJobRejected(int ioError)
{
    LOGM_DEBUG(TAG, "Ack received for SubscribeToStartNextJobRejected with code {%d}", ioError);
    if (ioError)
    {
        // TODO We need to implement a strategy for what do when our subscription fails
        string errorMessage = "Encountered an ioError while attempting to subscribe to StartNextJobRejected";
        LOG_ERROR(TAG, errorMessage.c_str());
        baseNotifier->onError(this, ClientBaseErrorNotification::SUBSCRIPTION_FAILED, errorMessage);
    }
}

void JobsFeature::ackUpdateJobExecutionStatus(int ioError)
{
    LOGM_DEBUG(TAG, "Ack received for PublishUpdateJobExecutionStatus with code {%d}", ioError);
}

void JobsFeature::ackSubscribeToUpdateJobExecutionAccepted(int ioError)
{
    LOGM_DEBUG(TAG, "Ack received for SubscribeToUpdateJobExecutionAccepted with code {%d}", ioError);
    if (ioError)
    {
        // TODO We need to implement a strategy for what do when our subscription fails
        string errorMessage = "Encountered an ioError while attempting to subscribe to UpdateJobExecutionAccepted";
        LOG_ERROR(TAG, errorMessage.c_str());
        baseNotifier->onError(this, ClientBaseErrorNotification::SUBSCRIPTION_FAILED, errorMessage);
    }
    updateAcceptedPromise.set_value(ioError);
}

void JobsFeature::ackSubscribeToUpdateJobExecutionRejected(int ioError)
{
    LOGM_DEBUG(TAG, "Ack received for SubscribeToUpdateJobExecutionRejected with code {%d}", ioError);
    if (ioError)
    {
        // TODO We need to implement a strategy for what do when our subscription fails
        string errorMessage = "Encountered an ioError while attempting to subscribe to UpdateJobExecutionRejected";
        LOG_ERROR(TAG, errorMessage.c_str());
        baseNotifier->onError(this, ClientBaseErrorNotification::SUBSCRIPTION_FAILED, errorMessage);
    }
    updateRejectedPromise.set_value(ioError);
}

/** Publishes a request to start the next pending job. In order to receive the response message,
 * subscribeToGetPendingJobs() must have been called successfully before this.
 */
void JobsFeature::publishStartNextPendingJobExecutionRequest()
{
    LOG_DEBUG(TAG, "Publishing startNextPendingJobExecutionRequest");
    StartNextPendingJobExecutionRequest startNextRequest;
    startNextRequest.ThingName = thingName.c_str();
    jobsClient->PublishStartNextPendingJobExecution(
        startNextRequest,
        AWS_MQTT_QOS_AT_LEAST_ONCE,
        std::bind(&JobsFeature::ackStartNextPendingJobPub, this, std::placeholders::_1));
}

/**
 * Creates the required topic subscriptions to enable delivery of the response message associated with
 * publishing a request to Start the next pending job execution
 */
void JobsFeature::subscribeToStartNextPendingJobExecution()
{
    LOG_DEBUG(TAG, "Attempting to subscribe to startNextPendingJobExecution accepted and rejected");
    StartNextPendingJobExecutionSubscriptionRequest startNextSub;
    startNextSub.ThingName = thingName.c_str();
    jobsClient->SubscribeToStartNextPendingJobExecutionAccepted(
        startNextSub,
        AWS_MQTT_QOS_AT_LEAST_ONCE,
        std::bind(&JobsFeature::startNextPendingJobReceivedHandler, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&JobsFeature::ackSubscribeToStartNextJobAccepted, this, std::placeholders::_1));
    jobsClient->SubscribeToStartNextPendingJobExecutionRejected(
        startNextSub,
        AWS_MQTT_QOS_AT_LEAST_ONCE,
        std::bind(&JobsFeature::startNextPendingJobRejectedHandler, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&JobsFeature::ackSubscribeToStartNextJobRejected, this, std::placeholders::_1));
}

/**
 * As the Jobs feature executes incoming jobs, the next pending job for this thing will change. By subscribing to
 * the topic associated with the NextJobExecutionChanged, we no longer need to poll for new jobs and instead can
 * be notified that there is new work to do.
 */
void JobsFeature::subscribeToNextJobChangedEvents()
{
    LOG_DEBUG(TAG, "Attempting to subscribe to nextJobChanged events");
    NextJobExecutionChangedSubscriptionRequest nextJobSub;
    nextJobSub.ThingName = thingName.c_str();
    jobsClient->SubscribeToNextJobExecutionChangedEvents(
        nextJobSub,
        AWS_MQTT_QOS_AT_LEAST_ONCE,
        std::bind(&JobsFeature::nextJobChangedHandler, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&JobsFeature::ackSubscribeToNextJobChanged, this, std::placeholders::_1));
}

void JobsFeature::subscribeToUpdateJobExecutionStatusAccepted(const string &jobId)
{
    LOGM_DEBUG(TAG, "Attempting to subscribe to updateJobExecutionStatusAccepted for jobId %s", jobId.c_str());
    UpdateJobExecutionSubscriptionRequest request;
    request.ThingName = thingName.c_str();
    request.JobId = jobId.c_str();
    jobsClient->SubscribeToUpdateJobExecutionAccepted(
        request,
        AWS_MQTT_QOS_AT_LEAST_ONCE,
        std::bind(
            &JobsFeature::updateJobExecutionStatusAcceptedHandler, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&JobsFeature::ackSubscribeToUpdateJobExecutionAccepted, this, std::placeholders::_1));

    if (std::future_status::timeout == updateAcceptedPromise.get_future().wait_for(std::chrono::seconds(10)))
    {
        ostringstream errorMessage;
        errorMessage
            << "Timed out while waiting for acknowledgement of subscription to UpdateJobExecutionStatusAccepted";
        LOG_ERROR(TAG, errorMessage.str().c_str());
        baseNotifier->onError(this, ClientBaseErrorNotification::SUBSCRIPTION_FAILED, errorMessage.str());
    }
}

void JobsFeature::subscribeToUpdateJobExecutionStatusRejected(const string &jobId)
{
    LOGM_DEBUG(TAG, "Attempting to subscribe to updateJobExecutionStatusRejected for jobId %s", jobId.c_str());
    UpdateJobExecutionSubscriptionRequest request;
    request.ThingName = thingName.c_str();
    request.JobId = jobId.c_str();
    jobsClient->SubscribeToUpdateJobExecutionRejected(
        request,
        AWS_MQTT_QOS_AT_LEAST_ONCE,
        std::bind(
            &JobsFeature::updateJobExecutionStatusRejectedHandler, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&JobsFeature::ackSubscribeToUpdateJobExecutionRejected, this, std::placeholders::_1));

    if (std::future_status::timeout == updateRejectedPromise.get_future().wait_for(std::chrono::seconds(10)))
    {
        ostringstream errorMessage;
        errorMessage
            << "Timed out while waiting for acknowledgement of subscription to UpdateJobExecutionStatusRejected";
        LOG_ERROR(TAG, errorMessage.str().c_str());
        baseNotifier->onError(this, ClientBaseErrorNotification::SUBSCRIPTION_FAILED, errorMessage.str());
    }
}

/**
 * Upon receipt of the PendingJobs message, this handler method will attempt to add the first available job to the
 * EventQueue.
 */
void JobsFeature::startNextPendingJobReceivedHandler(StartNextJobExecutionResponse *response, int ioError)
{
    if (ioError)
    {
        LOGM_ERROR(TAG, "Encountered ioError %d within startNextPendingJobReceivedHandler", ioError);
        return;
    }
    if (response->Execution.has_value())
    {
        if (needStop.load())
        {
            LOG_WARN(TAG, "Received new job but JobsFeature is stopped");
            ostringstream jobMessage;
            jobMessage << "Incoming " << Iotjobs::JobStatusMarshaller::ToString(response->Execution->Status.value())
                       << " job: " << response->Execution->JobId->c_str();
            baseNotifier->onError(this, ClientBaseErrorNotification::MESSAGE_RECEIVED_AFTER_SHUTDOWN, jobMessage.str());
            jobMessage << "Incoming " << Iotjobs::JobStatusMarshaller::ToString(response->Execution->Status.value())
                       << " job: " << response->Execution->JobId->c_str();
            baseNotifier->onError(this, ClientBaseErrorNotification::MESSAGE_RECEIVED_AFTER_SHUTDOWN, jobMessage.str());
            return;
        }
        else
        {
            if (!isDuplicateNotification(response->Execution.value()))
            {
                handlingJob.store(true);

                copyJobsNotification(response->Execution.value());
                initJob(response->Execution.value());
            }
        }
    }
    else
    {
        LOG_INFO(TAG, "No pending jobs are scheduled, waiting for the next incoming job");
    }
}

void JobsFeature::startNextPendingJobRejectedHandler(RejectedError *rejectedError, int ioError)
{
    if (ioError)
    {
        LOGM_ERROR(TAG, "Encountered ioError %d within startNextPendingJobRejectedHandler", ioError);
        return;
    }

    if (rejectedError->Message.has_value())
    {
        LOGM_ERROR(TAG, "startNextPendingJob rejected: %s", rejectedError->Message->c_str());
    }
}

void JobsFeature::nextJobChangedHandler(NextJobExecutionChangedEvent *event, int ioError)
{
    if (ioError)
    {
        LOGM_ERROR(TAG, "Encountered ioError %d within nextJobChangedHandler", ioError);
        return;
    }

    if (event->Execution.has_value())
    {
        if (needStop.load())
        {
            LOG_WARN(TAG, "Received new job but JobsFeature is stopped");
            ostringstream jobMessage;
            jobMessage << "Incoming " << Iotjobs::JobStatusMarshaller::ToString(event->Execution->Status.value())
                       << " job: " << event->Execution->JobId->c_str();
            baseNotifier->onError(this, ClientBaseErrorNotification::MESSAGE_RECEIVED_AFTER_SHUTDOWN, jobMessage.str());
            return;
        }
        else
        {
            // Check to see if this is a duplicate notification
            if (!isDuplicateNotification(event->Execution.value()))
            {
                handlingJob.store(true);

                copyJobsNotification(event->Execution.value());
                initJob(event->Execution.value());
            }
        }
    }
    else
    {
        LOG_INFO(TAG, "No pending jobs are scheduled, waiting for the next incoming job");
    }
}

void JobsFeature::updateJobExecutionStatusAcceptedHandler(Iotjobs::UpdateJobExecutionResponse *response, int ioError)
{
    if (ioError)
    {
        LOGM_ERROR(TAG, "Encountered ioError %d within updateJobExecutionStatusAcceptedHandler", ioError);
        return;
    }

    if (!response->ClientToken.has_value())
    {
        LOG_WARN(TAG, "Received an UpdateJobExecutionResponse with no ClientToken! Unable to update promise");
        return;
    }

    Aws::Crt::String clientToken = response->ClientToken.value();
    unique_lock<mutex> readLock(updateJobExecutionPromisesLock);
    auto keyValuePair = updateJobExecutionPromises.find(clientToken);
    if (keyValuePair == updateJobExecutionPromises.end())
    {
        LOGM_ERROR(TAG, "Could not find matching promise for ClientToken: %s", clientToken.c_str());
        return;
    }

    LOGM_DEBUG(TAG, "Removing ClientToken %s from the updateJobExecution promises map", clientToken.c_str());
    keyValuePair->second.set_value(ACCEPTED);
}

void JobsFeature::updateJobExecutionStatusRejectedHandler(Iotjobs::RejectedError *rejectedError, int ioError)
{
    if (ioError)
    {
        // Allow this proceed so it can be used to set the promise value and handle at the origin
        LOGM_ERROR(TAG, "Encountered ioError %d within updateJobExecutionStatusRejectedHandler", ioError);
    }

    if (!rejectedError->ClientToken || !rejectedError->ClientToken.has_value())
    {
        LOG_WARN(TAG, "Received an UpdateJobExecution rejected error with no ClientToken! Unable to update promise");
        return;
    }
    UpdateJobExecutionResponseType responseCode = NON_RETRYABLE_ERROR;
    Iotjobs::RejectedErrorCode rejectedErrorCode = rejectedError->Code.value();

    if (rejectedErrorCode != Iotjobs::RejectedErrorCode::RequestThrottled &&
        rejectedErrorCode != Iotjobs::RejectedErrorCode::ResourceNotFound &&
        rejectedErrorCode != Iotjobs::RejectedErrorCode::InternalError)
    {
        responseCode = RETRYABLE_ERROR;
    }

    Aws::Crt::String clientToken = rejectedError->ClientToken.value();
    unique_lock<mutex> readLock(updateJobExecutionPromisesLock);
    if (updateJobExecutionPromises.find(clientToken) == updateJobExecutionPromises.end())
    {
        LOGM_ERROR(TAG, "Could not find matching promise for ClientToken: %s", clientToken.c_str());
    }

    updateJobExecutionPromises.at(clientToken).set_value(responseCode);
}

void JobsFeature::publishUpdateJobExecutionStatus(
    JobExecutionData data,
    JobExecutionStatusInfo statusInfo,
    function<void(void)> onCompleteCallback)
{
    LOG_DEBUG(TAG, "Attempting to update job execution status!");

    Aws::Crt::Map<Aws::Crt::String, Aws::Crt::String> statusDetails;

    if (!statusInfo.reason.empty())
    {
        statusDetails["reason"] = statusInfo.reason.substr(0, MAX_STATUS_DETAIL_LENGTH).c_str();
    }

    if (!statusInfo.stdoutput.empty())
    {
        // We want the most recent output since we can only include 1024 characters in the job execution update
        int startPos = statusInfo.stdoutput.size() > MAX_STATUS_DETAIL_LENGTH
                           ? statusInfo.stdoutput.size() - MAX_STATUS_DETAIL_LENGTH
                           : 0;
        // TODO We need to add filtering of invalid characters for the status details that may come from weird
        // process output. The valid values for a statusDetail value are '[^\p{C}]+ which translates into
        // "everything other than invisible control characters and unused code points" (See
        // http://www.unicode.org/reports/tr18/#General_Category_Property)

        // NOTE(marcoaz): Aws::Crt::String does not convert from std::string
        // cppcheck-suppress danglingTemporaryLifetime
        statusDetails["stdout"] = statusInfo.stdoutput.substr(startPos, statusInfo.stdoutput.size()).c_str();
    }

    if (!statusInfo.stderror.empty())
    {
        int startPos = statusInfo.stderror.size() > MAX_STATUS_DETAIL_LENGTH
                           ? statusInfo.stderror.size() - MAX_STATUS_DETAIL_LENGTH
                           : 0;
        // NOTE(marcoaz): Aws::Crt::String does not convert from std::string
        // cppcheck-suppress danglingTemporaryLifetime
        statusDetails["stderr"] = statusInfo.stderror.substr(startPos, statusInfo.stderror.size()).c_str();
    }

    // NOTE(marcoaz): statusDetails is captured by value
    // cppcheck-suppress danglingTemporaryLifetime
    publishUpdateJobExecutionStatusWithRetry(data, statusInfo, statusDetails, onCompleteCallback);
}

void JobsFeature::publishUpdateJobExecutionStatusWithRetry(
    Aws::Iotjobs::JobExecutionData data,
    JobsFeature::JobExecutionStatusInfo statusInfo,
    Aws::Crt::Map<Aws::Crt::String, Aws::Crt::String> statusDetails,
    std::function<void(void)> onCompleteCallback)
{
    /** When we update the job execution status, we need to perform an exponential
     * backoff in case our request gets throttled. Otherwise, if we never properly
     * update the job execution status, we'll never receive the next job
     */
    Retry::ExponentialRetryConfig retryConfig = {10 * 1000, 640 * 1000, -1, &needStop};
    if (needStop.load())
    {
        // If we need to stop the Jobs feature, then we're making a best-effort attempt here
        // to update the job execution status prior to shutting down rather than infinite backoff
        retryConfig.maxRetries = 3;
        retryConfig.needStopFlag = nullptr;
    }

    auto publishLambda = [this, data, statusInfo, statusDetails]() -> bool {
        // We first need to make sure that we haven't previously leaked any promises into our map
        unique_lock<mutex> leakLock(updateJobExecutionPromisesLock);
        for (auto keyPromise = updateJobExecutionPromises.cbegin(); keyPromise != updateJobExecutionPromises.cend();
             /** no increment here **/)
        {
            if (keyPromise->second.isExpired())
            {
                LOGM_DEBUG(
                    TAG,
                    "Removing expired promise for ClientToken %s from the updateJobExecution promise map",
                    keyPromise->first.c_str());
                keyPromise = updateJobExecutionPromises.erase(keyPromise);
            }
            else
            {
                ++keyPromise;
            }
        }
        leakLock.unlock();

        UpdateJobExecutionRequest request;
        request.JobId = data.JobId->c_str();
        request.ThingName = this->thingName.c_str();
        request.Status = statusInfo.status;
        request.StatusDetails = statusDetails;

        // Create a unique client token each time we attempt the request since the promise has to be fresh
        string clientToken = UniqueString::GetRandomToken(10);
        request.ClientToken = Aws::Crt::Optional<Aws::Crt::String>(clientToken.c_str());
        unique_lock<mutex> writeLock(updateJobExecutionPromisesLock);
        this->updateJobExecutionPromises.insert(
            std::pair<Aws::Crt::String, EphemeralPromise<UpdateJobExecutionResponseType>>(
                clientToken.c_str(),
                EphemeralPromise<UpdateJobExecutionResponseType>(std::chrono::milliseconds(15 * 1000))));
        writeLock.unlock();
        LOGM_DEBUG(
            TAG,
            "Created EphemeralPromise for ClientToken %s in the updateJobExecution promises map",
            clientToken.c_str());

        this->jobsClient->PublishUpdateJobExecution(
            request,
            AWS_MQTT_QOS_AT_LEAST_ONCE,
            std::bind(&JobsFeature::ackUpdateJobExecutionStatus, this, std::placeholders::_1));
        unique_lock<mutex> futureLock(updateJobExecutionPromisesLock);
        future<UpdateJobExecutionResponseType> updateFuture =
            this->updateJobExecutionPromises.at(clientToken.c_str()).get_future();
        futureLock.unlock();
        bool finished = false;
        // Although this entire block will be retried based on the retryConfig, we're only waiting for a maximum of 10
        // seconds for each individual response
        if (std::future_status::timeout == updateFuture.wait_for(std::chrono::seconds(10)))
        {
            LOGM_WARN(TAG, "Timeout waiting for ack from PublishUpdateJobExecution for job %s", data.JobId->c_str());
        }
        else
        {
            int responseCode = updateFuture.get();
            if (responseCode != ACCEPTED)
            {
                if (responseCode == NON_RETRYABLE_ERROR)
                {
                    LOGM_ERROR(
                        TAG,
                        "Received a non-retryable error response after publishing an UpdateJobExecution request for "
                        "job %s",
                        data.JobId->c_str());
                    finished = true;
                }
                else
                {
                    LOGM_WARN(
                        TAG,
                        "Received a retryable error response after publishing an UpdateJobExecution request for job %s",
                        data.JobId->c_str());
                }
            }
            else
            {
                LOGM_DEBUG(TAG, "Success response after UpdateJobExecution for job %s", data.JobId->c_str());
                finished = true;
            }
        }
        unique_lock<mutex> eraseLock(updateJobExecutionPromisesLock);
        this->updateJobExecutionPromises.erase(clientToken.c_str());
        return finished;
    };
    std::thread updateJobExecutionThread([retryConfig, publishLambda, onCompleteCallback] {
        Retry::exponentialBackoff(retryConfig, publishLambda, onCompleteCallback);
    });
    updateJobExecutionThread.detach();
}

void JobsFeature::copyJobsNotification(Iotjobs::JobExecutionData job)
{
    unique_lock<mutex> copyNotificationLock(latestJobsNotificationLock);
    latestJobsNotification.JobId = job.JobId.value();
    latestJobsNotification.JobDocument = job.JobDocument.value();
    latestJobsNotification.ExecutionNumber = job.ExecutionNumber.value();
}

bool JobsFeature::isDuplicateNotification(JobExecutionData job)
{
    unique_lock<mutex> readLatestNotificationLock(latestJobsNotificationLock);
    if (!latestJobsNotification.JobId.has_value())
    {
        // We have not seen a job yet
        LOG_DEBUG(TAG, "We have not seen a job yet, this is not a duplicate job notification");
        return false;
    }

    if (strcmp(job.JobId.value().c_str(), latestJobsNotification.JobId.value().c_str()) != 0)
    {
        LOG_DEBUG(TAG, "Job ids differ");
        return false;
    }

    if (strcmp(
            job.JobDocument.value().View().WriteCompact().c_str(),
            latestJobsNotification.JobDocument.value().View().WriteCompact().c_str()) != 0)
    {
        LOG_DEBUG(TAG, "Job document differs");
        return false;
    }

    if (job.ExecutionNumber.value() != latestJobsNotification.ExecutionNumber.value())
    {
        LOG_DEBUG(TAG, "Execution number differs");
        return false;
    }

    LOG_DEBUG(TAG, "Encountered a duplicate job notification");
    return true;
}

void JobsFeature::initJob(const JobExecutionData &job)
{
    auto shutdownHandler = [this]() -> void {
        handlingJob.store(false);
        if (needStop.load())
        {
            LOGM_INFO(TAG, "Shutting down %s now that job execution is complete", getName().c_str());
            baseNotifier->onEvent((Feature *)this, ClientBaseEventNotification::FEATURE_STOPPED);
        }
    };

    Aws::Crt::JsonView jobDoc = job.JobDocument->View();
    PlainJobDocument jobDocument;
    // reject job document based on the validation status
    jobDocument.LoadFromJobDocument(jobDoc);
    if (!jobDocument.Validate())
    {
        LOG_ERROR(TAG, "Unable to execute job, invalid job document provided!");
        publishUpdateJobExecutionStatus(
            job,
            JobExecutionStatusInfo(
                Iotjobs::JobStatus::REJECTED, "Unable to execute job, invalid job document provided!", "", ""),
            shutdownHandler);
        return;
    }
    publishUpdateJobExecutionStatus(job, JobExecutionStatusInfo(Iotjobs::JobStatus::IN_PROGRESS));
    executeJob(job, jobDocument);
}

void JobsFeature::executeJob(const Iotjobs::JobExecutionData &job, const PlainJobDocument &jobDocument)
{
    LOGM_INFO(TAG, "Executing job: %s", job.JobId->c_str());

    auto shutdownHandler = [this]() -> void {
        handlingJob.store(false);
        if (needStop.load())
        {
            LOGM_INFO(TAG, "Shutting down %s now that job execution is complete", getName().c_str());
            baseNotifier->onEvent((Feature *)this, ClientBaseEventNotification::FEATURE_STOPPED);
        }
    };
    // TODO: Add support for checking condition
    auto runJob = [this, job, jobDocument, shutdownHandler]() {
        auto engine = createJobEngine();
        // execute all action steps in sequence as provided in job document
        int executionStatus = engine->exec_steps(jobDocument, jobHandlerDir);
        string reason = engine->getReason(executionStatus);

        LOG_INFO(TAG, Sanitize(reason).c_str());

        if (engine->hasErrors())
        {
            LOG_WARN(TAG, "JobEngine reported receiving errors from STDERR");
        }

        string standardOut;
        if (jobDocument.includeStdOut)
        {
            standardOut = engine->getStdOut();
        }
        else
        {
            LOG_DEBUG(TAG, "Not including stdout with the status details");
        }
        JobStatus status;
        if (!executionStatus)
        {
            LOG_INFO(TAG, "Job executed successfully!");
            status = JobStatus::SUCCEEDED;
        }
        else
        {
            LOG_WARN(TAG, "Job execution failed!");
            status = JobStatus::FAILED;
        }
        publishUpdateJobExecutionStatus(
            job, JobExecutionStatusInfo(status, reason, standardOut, engine->getStdErr()), shutdownHandler);
    };
    thread jobEngineThread(runJob);
    jobEngineThread.detach();
}

void JobsFeature::runJobs()
{
    LOGM_INFO(TAG, "Running %s!", getName().c_str());

    jobsClient = createJobsClient();

    // Create subscriptions to important MQTT topics
    subscribeToStartNextPendingJobExecution();
    subscribeToNextJobChangedEvents();

    // We want to be notified on any response to an UpdateJobExecution call
    subscribeToUpdateJobExecutionStatusAccepted("+");
    subscribeToUpdateJobExecutionStatusRejected("+");

    publishStartNextPendingJobExecutionRequest();
}

int JobsFeature::init(
    shared_ptr<Mqtt::MqttConnection> connection,
    shared_ptr<ClientBaseNotifier> notifier,
    const PlainConfig &config)
{
    mqttConnection = connection;
    baseNotifier = notifier;
    thingName = config.thingName->c_str();

    wordexp_t word;
    if (!config.jobs.handlerDir.empty())
    {
        wordexp(config.jobs.handlerDir.c_str(), &word, 0);
        jobHandlerDir = word.we_wordv[0];
    }
    else
    {
        wordexp(DEFAULT_JOBS_HANDLER_DIR.c_str(), &word, 0);
        jobHandlerDir = word.we_wordv[0];
    }
    wordfree(&word);

    return 0;
}

int JobsFeature::start()
{
    thread jobs_thread(&JobsFeature::runJobs, this);
    jobs_thread.detach();

    baseNotifier->onEvent((Feature *)this, ClientBaseEventNotification::FEATURE_STARTED);
    return 0;
}

int JobsFeature::stop()
{
    needStop.store(true);
    if (!handlingJob.load())
    {
        baseNotifier->onEvent((Feature *)this, ClientBaseEventNotification::FEATURE_STOPPED);
    }

    return 0;
}

std::shared_ptr<AbstractIotJobsClient> JobsFeature::createJobsClient()
{
    return std::shared_ptr<AbstractIotJobsClient>(new IotJobsClientWrapper(mqttConnection));
}

std::shared_ptr<JobEngine> JobsFeature::createJobEngine()
{
    return std::shared_ptr<JobEngine>(new JobEngine());
}
