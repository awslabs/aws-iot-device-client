// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_JOBSFEATURE_H
#define DEVICE_CLIENT_JOBSFEATURE_H

#include <aws/iot/MqttClient.h>
#include <aws/iotjobs/IotJobsClient.h>
#include <aws/iotjobs/JobExecutionData.h>

#include "../Feature.h"
#include "../ClientBaseNotifier.h"
#include "../SharedCrtResourceManager.h"
#include "EphemeralPromise.h"

namespace Aws {
    namespace Iot {
        namespace DeviceClient {
            class JobsFeature : public Feature {
                private:
                    const char * TAG = "JobsFeature.cpp";
                    const char * JOB_ATTR_OP = "operation";
                    const char * JOB_ATTR_ARGS = "args";
                    const char * JOB_ATTR_ALLOW_STDERR = "allowStdErr";
                    const char * JOB_ATTR_PATH = "path";
                    const char * DEFAULT_PATH_KEYWORD = "default";
                    const std::string DEFAULT_JOBS_HANDLER_DIR = "/etc/aws-iot-device-client/handlers/";
                    const size_t MAX_STATUS_DETAIL_LENGTH = 1024;
                    const int UPDATE_JOB_EXECUTION_REJECTED_CODE = -1;

                    bool needStop = false;
                    bool handlingJob = false;
                    std::mutex canStopLock;

                    std::mutex updateJobExecutionPromisesLock;
                    Aws::Crt::Map<Aws::Crt::String, Aws::Iot::DeviceClient::Jobs::EphemeralPromise<int>> updateJobExecutionPromises;
                    std::shared_ptr<SharedCrtResourceManager> mqttManager;
                    std::shared_ptr<ClientBaseNotifier> baseNotifier;
                    std::unique_ptr<Aws::Iotjobs::IotJobsClient> jobsClient;

                    std::string thingName;
                    std::string jobHandlerDir = DEFAULT_JOBS_HANDLER_DIR;
                    bool includeStdoutInUpdates = false;

                    // Ack handlers
                    void ackSubscribeToNextJobChanged(int ioError);
                    void ackSubscribeToStartNextJobAccepted(int ioError);
                    void ackSubscribeToStartNextJobRejected(int ioError);
                    void ackStartNextPendingJobPub(int ioError);
                    void ackUpdateJobExecutionStatus(int ioError);
                    void ackSubscribeToUpdateJobExecutionAccepted(int ioError);
                    void ackSubscribeToUpdateJobExecutionRejected(int ioError);

                    // Outgoing Mqtt messages/topic subscriptions
                    void publishStartNextPendingJobExecutionRequest();

                    struct JobExecutionStatusInfo {
                        Aws::Iotjobs::JobStatus status;
                        std::string reason;
                        std::string stdoutput;
                        std::string stderror;
                    };

                    void publishUpdateJobExecutionStatus(Aws::Iotjobs::JobExecutionData data, JobExecutionStatusInfo statusInfo);
                    void subscribeToStartNextPendingJobExecution();
                    void subscribeToNextJobChangedEvents();
                    void subscribeToUpdateJobExecutionStatusAccepted(std::string jobId);
                    void subscribeToUpdateJobExecutionStatusRejected(std::string jobId);

                    // Incoming Mqtt message handlers
                    void startNextPendingJobReceivedHandler(Iotjobs::StartNextJobExecutionResponse* response, int ioError);
                    void startNextPendingJobRejectedHandler(Iotjobs::RejectedError* rejectedError, int ioError);
                    void nextJobChangedHandler(Iotjobs::NextJobExecutionChangedEvent *event, int ioErr);
                    void updateJobExecutionStatusAcceptedHandler(Iotjobs::UpdateJobExecutionResponse* response, int ioError);
                    void updateJobExecutionStatusRejectedHandler(Iotjobs::RejectedError* rejectedError, int ioError);

                    // Process a Job
                    void executeJob(Iotjobs::JobExecutionData job);
                    // Begin running the Jobs feature
                    void runJobs();
                public:
                    virtual std::string get_name();
                    virtual int init(std::shared_ptr<SharedCrtResourceManager> manager, std::shared_ptr<ClientBaseNotifier> notifier, const PlainConfig &config);

                    // Interface methods defined in Feature.h
                    virtual int start();
                    virtual int stop();
            };
        }
    }
}


#endif //DEVICE_CLIENT_JOBSFEATURE_H
