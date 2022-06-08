// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/Feature.h"
#include "../../source/jobs/JobsFeature.h"
#include <aws/iotjobs/IotJobsClient.h>
#include <aws/iotjobs/NextJobExecutionChangedSubscriptionRequest.h>
#include <aws/iotjobs/StartNextPendingJobExecutionRequest.h>
#include <aws/iotjobs/StartNextPendingJobExecutionSubscriptionRequest.h>
#include <aws/iotjobs/UpdateJobExecutionRequest.h>
#include <aws/iotjobs/UpdateJobExecutionSubscriptionRequest.h>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <aws/iotjobs/RejectedError.h>
#include <aws/iotjobs/StartNextJobExecutionResponse.h>
#include <aws/iotjobs/UpdateJobExecutionResponse.h>

using namespace std;
using namespace testing;
using namespace Aws;
using namespace Aws::Crt;
using namespace Aws::Iotjobs;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::Jobs;

PlainConfig getSimpleConfig()
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "key": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test-file",
    "thing-name": "thing-name value",
    "logging": {
        "level": "ERROR",
        "type": "file",
        "file": "./aws-iot-device-client.log"
    },
    "jobs": {
        "enabled": true
    }
})";

    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    return config;
}

JobExecutionData getSampleJobExecution(std::string jobId, int executionNumber)
{
    constexpr char jsonString[] = R"(
{
    "version": "1.0",
    "jobId": "test-job-id",
    "includeStdOut": "true",
    "conditions": [{
                    "key" : "operatingSystem",
                    "value": ["ubuntu", "redhat"],
                     "type": "stringEqual"
                 },
                 {
                    "key" : "OS",
                     "value": ["16.0"],
                     "type": "stringEqual"
    }],
    "steps": [{
            "action": {
                "name": "downloadJobHandler",
                "type": "runHandler",
                "input": {
                    "handler": "download-file.sh",
                    "args": ["presignedUrl", "/tmp/aws-iot-device-client/"],
                    "path": "path to handler"
                },
                "runAsUser": "user1",
                "allowStdErr": 8,
                "ignoreStepFailure": "true"
            }
        },
        {
            "action": {
                "name": "installApplicationAndReboot",
                "type": "runHandler",
                "input": {
                    "handler": "install-app.sh",
                    "args": [
                        "applicationName",
                        "active"
                    ],
                    "path": "path to handler"
                },
                "runAsUser": "user1",
                "allowStdErr": 8,
                "ignoreStepFailure": "true"
            }
        },
        {
            "action": {
                "name": "validateAppStatus",
                "type": "runHandler",
                "input": {
                    "handler": "validate-app-status.sh",
                    "args": [
                        "applicationName",
                        "active"
                    ],
                    "path": "path to handler"
                },
                "runAsUser": "user1",
                "allowStdErr": 8,
                "ignoreStepFailure": "true"
            }
        }
    ],
    "finalStep": {
        "action": {
            "name": "deleteDownloadedHandler",
            "type": "runHandler",
            "input": {
                 "handler": "validate-app-status.sh",
                 "args": [
                    "applicationName",
                    "active"
                ],
                "path": "path to handler"
             },
            "runAsUser": "user1",
            "allowStdErr": 8,
            "ignoreStepFailure": "true"
        }
    }
})";

    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    JobExecutionData jobExecutionData(jsonView);
    jobExecutionData.JobDocument = Aws::Crt::Optional<JsonObject>(jsonObject);
    jobExecutionData.JobId =
        Aws::Crt::Optional<basic_string<char, char_traits<char>, StlAllocator<char>>>(jobId.c_str());
    jobExecutionData.ExecutionNumber = Aws::Crt::Optional<int64_t>(executionNumber);

    return jobExecutionData;
}

class MockJobsClient : public AbstractIotJobsClient
{
  public:
    MOCK_METHOD(
        void,
        PublishStartNextPendingJobExecution,
        (const StartNextPendingJobExecutionRequest &request,
         Aws::Crt::Mqtt::QOS qos,
         const Iotjobs::OnPublishComplete &onPubAck),
        (override));
    MOCK_METHOD(
        void,
        SubscribeToStartNextPendingJobExecutionAccepted,
        (const Iotjobs::StartNextPendingJobExecutionSubscriptionRequest &request,
         Aws::Crt::Mqtt::QOS qos,
         const Iotjobs::OnSubscribeToStartNextPendingJobExecutionAcceptedResponse &handler,
         const Iotjobs::OnSubscribeComplete &onSubAck),
        (override));
    MOCK_METHOD(
        void,
        SubscribeToStartNextPendingJobExecutionRejected,
        (const Iotjobs::StartNextPendingJobExecutionSubscriptionRequest &request,
         Aws::Crt::Mqtt::QOS qos,
         const Iotjobs::OnSubscribeToStartNextPendingJobExecutionRejectedResponse &handler,
         const Iotjobs::OnSubscribeComplete &onSubAck),
        (override));
    MOCK_METHOD(
        void,
        SubscribeToNextJobExecutionChangedEvents,
        (const Iotjobs::NextJobExecutionChangedSubscriptionRequest &request,
         Aws::Crt::Mqtt::QOS qos,
         const Iotjobs::OnSubscribeToNextJobExecutionChangedEventsResponse &handler,
         const Iotjobs::OnSubscribeComplete &onSubAck),
        (override));
    MOCK_METHOD(
        void,
        SubscribeToUpdateJobExecutionAccepted,
        (const Iotjobs::UpdateJobExecutionSubscriptionRequest &request,
         Aws::Crt::Mqtt::QOS qos,
         const Iotjobs::OnSubscribeToUpdateJobExecutionAcceptedResponse &handler,
         const Iotjobs::OnSubscribeComplete &onSubAck),
        (override));
    MOCK_METHOD(
        void,
        SubscribeToUpdateJobExecutionRejected,
        (const Iotjobs::UpdateJobExecutionSubscriptionRequest &request,
         Aws::Crt::Mqtt::QOS qos,
         const Iotjobs::OnSubscribeToUpdateJobExecutionRejectedResponse &handler,
         const Iotjobs::OnSubscribeComplete &onSubAck),
        (override));
    MOCK_METHOD(
        void,
        PublishUpdateJobExecution,
        (const Iotjobs::UpdateJobExecutionRequest &request,
         Aws::Crt::Mqtt::QOS qos,
         const Iotjobs::OnPublishComplete &onPubAck),
        (override));
};

class MockNotifier : public Aws::Iot::DeviceClient::ClientBaseNotifier
{
  public:
    MOCK_METHOD(
        void,
        onEvent,
        (Aws::Iot::DeviceClient::Feature * feature, Aws::Iot::DeviceClient::ClientBaseEventNotification notification),
        (override));
    MOCK_METHOD(
        void,
        onError,
        (Aws::Iot::DeviceClient::Feature * feature,
         Aws::Iot::DeviceClient::ClientBaseErrorNotification notification,
         const std::string &message),
        (override));
};

class MockJobEngine : public JobEngine
{
  public:
    MOCK_METHOD(void, processCmdOutput, (int fd, bool isStdErr, int childPID), (override));
    MOCK_METHOD(int, exec_steps, (PlainJobDocument jobDocument, const std::string &jobHandlerDir), (override));
    MOCK_METHOD(int, hasErrors, (), (override));
    MOCK_METHOD(string, getReason, (int statusCode), (override));
    MOCK_METHOD(string, getStdOut, (), (override));
    MOCK_METHOD(string, getStdErr, (), (override));
};

class MockJobsFeature : public JobsFeature
{
  public:
    MockJobsFeature() : JobsFeature() {}
    void invokeRunJobs() { this->runJobs(); }
    MOCK_METHOD(shared_ptr<AbstractIotJobsClient>, createJobsClient, (), (override));
    MOCK_METHOD(shared_ptr<JobEngine>, createJobEngine, (), (override));
    MOCK_METHOD(
        void,
        publishUpdateJobExecutionStatusWithRetry,
        (Aws::Iotjobs::JobExecutionData data,
         JobsFeature::JobExecutionStatusInfo statusInfo,
         (Aws::Crt::Map<Aws::Crt::String, Aws::Crt::String> statusDetails),
         std::function<void(void)> onCompleteCallback),
        (override));
};

class TestJobsFeature : public ::testing::Test
{
  public:
    void SetUp()
    {
        ThingName = Aws::Crt::String("thing-name value");
        notifier = shared_ptr<MockNotifier>(new MockNotifier());
        config = getSimpleConfig();
        startNextJobExecutionResponse =
            std::unique_ptr<StartNextJobExecutionResponse>(new StartNextJobExecutionResponse());
        jobsMock = unique_ptr<MockJobsFeature>(new MockJobsFeature());
        mockClient = shared_ptr<MockJobsClient>(new MockJobsClient());
        mockEngine = shared_ptr<MockJobEngine>(new MockJobEngine());
    }
    Aws::Crt::String ThingName;
    shared_ptr<MockNotifier> notifier;
    PlainConfig config;
    unique_ptr<StartNextJobExecutionResponse> startNextJobExecutionResponse;
    unique_ptr<MockJobsFeature> jobsMock;
    shared_ptr<MockJobsClient> mockClient;
    shared_ptr<MockJobEngine> mockEngine;
};

MATCHER_P(ThingNameEq, ThingName, "Matcher ThingName for all Aws request Objects using Aws::Crt::String")
{
    return arg.ThingName.value() == ThingName;
}

MATCHER_P(StatusInfoEq, statusInfo, "Matches JobExecutionStatusInfo status")
{
    return arg.status == statusInfo.status && arg.reason == statusInfo.reason &&
           arg.stdoutput == statusInfo.stdoutput && arg.stderror == statusInfo.stderror;
}

MATCHER_P(JobExecutionEq, job, "Matches JobExecutionData JobId & ExecutionNumber")
{
    return arg.JobId.value() == job.JobId.value() && arg.ExecutionNumber.value() == job.ExecutionNumber.value();
}

TEST_F(TestJobsFeature, GetName)
{
    /**
     * Simple test for GetName
     */
    ASSERT_STREQ(jobsMock->getName().c_str(), "Jobs");
}

TEST_F(TestJobsFeature, Init)
{
    /**
     * Test init Jobs with null MqttConnection, Mock notifier, and PlainConfig
     */
    ASSERT_EQ(0, jobsMock->init(std::shared_ptr<Mqtt::MqttConnection>(), notifier, config));
}

TEST_F(TestJobsFeature, RunJobsHappy)
{
    /**
     * Inject a MockJobsClient into Jobs Feature and invoke RunJobs
     * Verifies Subscription Requests to IotJobsClient and invokes SubAck Callback functions
     * Invokes SubAck callbacks rather than elaborate argument matching
     */
    EXPECT_CALL(*jobsMock, createJobsClient()).Times(1).WillOnce(Return(mockClient));

    EXPECT_CALL(
        *mockClient,
        SubscribeToStartNextPendingJobExecutionAccepted(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(InvokeArgument<3>(0));
    EXPECT_CALL(
        *mockClient,
        SubscribeToStartNextPendingJobExecutionRejected(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(InvokeArgument<3>(0));
    EXPECT_CALL(
        *mockClient, SubscribeToNextJobExecutionChangedEvents(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(InvokeArgument<3>(0));
    EXPECT_CALL(
        *mockClient, SubscribeToUpdateJobExecutionAccepted(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(InvokeArgument<3>(0));
    EXPECT_CALL(
        *mockClient, SubscribeToUpdateJobExecutionRejected(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(InvokeArgument<3>(0));
    EXPECT_CALL(*mockClient, PublishStartNextPendingJobExecution(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _))
        .Times(1)
        .WillOnce(InvokeArgument<2>(0));

    jobsMock->init(std::shared_ptr<Mqtt::MqttConnection>(), notifier, config);
    jobsMock->invokeRunJobs();
}

TEST_F(TestJobsFeature, ExecuteJobHappy)
{
    /**
     * Inject a MockJobsClient and MockJobEngine into Jobs Feature and invoke RunJobs
     * Verifies Subscription Requests to IotJobsClient and invokes SubAck Callback functions
     * Invokes the StartNextPendingJobExecution Handler with a Test JobExecution
     * Verifies JobExecution is updated to IN_PROGRESS then SUCCEEDED
     */
    const JobExecutionData job = getSampleJobExecution("job1", 1);
    startNextJobExecutionResponse->Execution = Aws::Crt::Optional<JobExecutionData>(job);

    // As JobEngine is run in a separate thread this is needed so that the tests wait for that thread to update JE
    std::promise<void> promise;
    auto setPromise = [&promise]() -> void { promise.set_value(); };

    string stdoutput = "test output";

    EXPECT_CALL(*jobsMock, createJobEngine()).Times(1).WillOnce(Return(mockEngine));
    EXPECT_CALL(*mockEngine, exec_steps(_, _)).WillOnce(Return(0));
    EXPECT_CALL(*mockEngine, hasErrors()).WillOnce(Return(1));
    EXPECT_CALL(*mockEngine, getReason(_)).WillOnce(Return(""));
    EXPECT_CALL(*mockEngine, getStdOut()).WillOnce(Return(stdoutput));
    EXPECT_CALL(*mockEngine, getStdErr()).WillOnce(Return(""));

    EXPECT_CALL(*jobsMock, createJobsClient()).Times(1).WillOnce(Return(mockClient));

    EXPECT_CALL(
        *mockClient,
        SubscribeToStartNextPendingJobExecutionAccepted(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(DoAll(InvokeArgument<3>(0), InvokeArgument<2>(startNextJobExecutionResponse.get(), 0)));
    EXPECT_CALL(
        *mockClient,
        SubscribeToStartNextPendingJobExecutionRejected(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(InvokeArgument<3>(0));
    EXPECT_CALL(
        *mockClient, SubscribeToNextJobExecutionChangedEvents(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(InvokeArgument<3>(0));
    EXPECT_CALL(
        *mockClient, SubscribeToUpdateJobExecutionAccepted(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(InvokeArgument<3>(0));
    EXPECT_CALL(
        *mockClient, SubscribeToUpdateJobExecutionRejected(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(InvokeArgument<3>(0));
    EXPECT_CALL(*mockClient, PublishStartNextPendingJobExecution(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _))
        .Times(1)
        .WillOnce(InvokeArgument<2>(0));

    EXPECT_CALL(
        *jobsMock,
        publishUpdateJobExecutionStatusWithRetry(
            JobExecutionEq(job),
            StatusInfoEq(JobsFeature::JobExecutionStatusInfo(Iotjobs::JobStatus::IN_PROGRESS, "", "", "")),
            IsEmpty(),
            IsNull()))
        .Times(1);
    EXPECT_CALL(
        *jobsMock,
        publishUpdateJobExecutionStatusWithRetry(
            JobExecutionEq(job),
            StatusInfoEq(JobsFeature::JobExecutionStatusInfo(Iotjobs::JobStatus::SUCCEEDED, "", stdoutput, "")),
            _,
            _))
        .WillOnce(InvokeWithoutArgs(setPromise));

    jobsMock->init(std::shared_ptr<Mqtt::MqttConnection>(), notifier, config);
    jobsMock->invokeRunJobs();

    EXPECT_EQ(std::future_status::ready, promise.get_future().wait_for(std::chrono::seconds(3)));
}

TEST_F(TestJobsFeature, ExecuteJobStderror)
{
    /**
     * Inject a MockJobsClient and MockJobEngine into Jobs Feature and invoke RunJobs
     * Verifies Subscription Requests to IotJobsClient and invokes SubAck Callback functions
     * Invokes the StartNextPendingJobExecution Handler with a Test JobExecution
     * Verifies JobExecution is updated to IN_PROGRESS then SUCCEEDED
     * Verifies stdout from JobEngine
     */
    const JobExecutionData job = getSampleJobExecution("job1", 1);
    startNextJobExecutionResponse->Execution = Aws::Crt::Optional<JobExecutionData>(job);

    // As JobEngine is run in a separate thread this is needed so that the tests wait for that thread to update JE
    std::promise<void> promise;
    auto setPromise = [&promise]() -> void { promise.set_value(); };

    string stderror = "error output";

    EXPECT_CALL(*jobsMock, createJobEngine()).Times(1).WillOnce(Return(mockEngine));

    EXPECT_CALL(*mockEngine, exec_steps(_, _)).WillOnce(Return(0));
    EXPECT_CALL(*mockEngine, hasErrors()).WillOnce(Return(1));
    EXPECT_CALL(*mockEngine, getReason(_)).WillOnce(Return(""));
    EXPECT_CALL(*mockEngine, getStdOut()).WillOnce(Return(""));
    EXPECT_CALL(*mockEngine, getStdErr()).WillOnce(Return(stderror));

    EXPECT_CALL(*jobsMock, createJobsClient()).Times(1).WillOnce(Return(mockClient));

    EXPECT_CALL(
        *mockClient,
        SubscribeToStartNextPendingJobExecutionAccepted(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(DoAll(InvokeArgument<3>(0), InvokeArgument<2>(startNextJobExecutionResponse.get(), 0)));
    EXPECT_CALL(
        *mockClient,
        SubscribeToStartNextPendingJobExecutionRejected(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(InvokeArgument<3>(0));
    EXPECT_CALL(
        *mockClient, SubscribeToNextJobExecutionChangedEvents(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(InvokeArgument<3>(0));
    EXPECT_CALL(
        *mockClient, SubscribeToUpdateJobExecutionAccepted(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(InvokeArgument<3>(0));
    EXPECT_CALL(
        *mockClient, SubscribeToUpdateJobExecutionRejected(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(InvokeArgument<3>(0));
    EXPECT_CALL(*mockClient, PublishStartNextPendingJobExecution(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _))
        .Times(1)
        .WillOnce(InvokeArgument<2>(0));

    EXPECT_CALL(
        *jobsMock,
        publishUpdateJobExecutionStatusWithRetry(
            JobExecutionEq(job),
            StatusInfoEq(JobsFeature::JobExecutionStatusInfo(Iotjobs::JobStatus::IN_PROGRESS, "", "", "")),
            IsEmpty(),
            IsNull()))
        .Times(1);
    EXPECT_CALL(
        *jobsMock,
        publishUpdateJobExecutionStatusWithRetry(
            JobExecutionEq(job),
            StatusInfoEq(JobsFeature::JobExecutionStatusInfo(Iotjobs::JobStatus::SUCCEEDED, "", "", stderror)),
            _,
            _))
        .WillOnce(InvokeWithoutArgs(setPromise));

    jobsMock->init(std::shared_ptr<Mqtt::MqttConnection>(), notifier, config);
    jobsMock->invokeRunJobs();

    EXPECT_EQ(std::future_status::ready, promise.get_future().wait_for(std::chrono::seconds(3)));
}

TEST_F(TestJobsFeature, ExecuteJobStdOutAndStderror)
{
    /**
     * Inject a MockJobsClient and MockJobEngine into Jobs Feature and invoke RunJobs
     * Verifies Subscription Requests to IotJobsClient and invokes SubAck Callback functions
     * Invokes the StartNextPendingJobExecution Handler with a Test JobExecution
     * Verifies JobExecution is updated to IN_PROGRESS then SUCCEEDED
     * Verifies stdout from JobEngine
     */
    const JobExecutionData job = getSampleJobExecution("job1", 1);
    startNextJobExecutionResponse->Execution = Aws::Crt::Optional<JobExecutionData>(job);

    // As JobEngine is run in a separate thread this is needed so that the tests wait for that thread to update JE
    std::promise<void> promise;
    auto setPromise = [&promise]() -> void { promise.set_value(); };

    string stdoutput = "test output";
    string stderror = "error output";

    EXPECT_CALL(*jobsMock, createJobEngine()).Times(1).WillOnce(Return(mockEngine));

    EXPECT_CALL(*mockEngine, exec_steps(_, _)).WillOnce(Return(0));
    EXPECT_CALL(*mockEngine, hasErrors()).WillOnce(Return(1));
    EXPECT_CALL(*mockEngine, getReason(_)).WillOnce(Return(""));
    EXPECT_CALL(*mockEngine, getStdOut()).WillOnce(Return(stdoutput));
    EXPECT_CALL(*mockEngine, getStdErr()).WillOnce(Return(stderror));

    EXPECT_CALL(*jobsMock, createJobsClient()).Times(1).WillOnce(Return(mockClient));

    EXPECT_CALL(
        *mockClient,
        SubscribeToStartNextPendingJobExecutionAccepted(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(DoAll(InvokeArgument<3>(0), InvokeArgument<2>(startNextJobExecutionResponse.get(), 0)));
    EXPECT_CALL(
        *mockClient,
        SubscribeToStartNextPendingJobExecutionRejected(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(InvokeArgument<3>(0));
    EXPECT_CALL(
        *mockClient, SubscribeToNextJobExecutionChangedEvents(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(InvokeArgument<3>(0));
    EXPECT_CALL(
        *mockClient, SubscribeToUpdateJobExecutionAccepted(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(InvokeArgument<3>(0));
    EXPECT_CALL(
        *mockClient, SubscribeToUpdateJobExecutionRejected(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(InvokeArgument<3>(0));
    EXPECT_CALL(*mockClient, PublishStartNextPendingJobExecution(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _))
        .Times(1)
        .WillOnce(InvokeArgument<2>(0));

    EXPECT_CALL(
        *jobsMock,
        publishUpdateJobExecutionStatusWithRetry(
            JobExecutionEq(job),
            StatusInfoEq(JobsFeature::JobExecutionStatusInfo(Iotjobs::JobStatus::IN_PROGRESS, "", "", "")),
            IsEmpty(),
            IsNull()))
        .Times(1);
    EXPECT_CALL(
        *jobsMock,
        publishUpdateJobExecutionStatusWithRetry(
            JobExecutionEq(job),
            StatusInfoEq(JobsFeature::JobExecutionStatusInfo(Iotjobs::JobStatus::SUCCEEDED, "", stdoutput, stderror)),
            _,
            _))
        .WillOnce(InvokeWithoutArgs(setPromise));

    jobsMock->init(std::shared_ptr<Mqtt::MqttConnection>(), notifier, config);
    jobsMock->invokeRunJobs();

    EXPECT_EQ(std::future_status::ready, promise.get_future().wait_for(std::chrono::seconds(3)));
}

TEST_F(TestJobsFeature, ExecuteJobDuplicateNotificaton)
{
    /**
     * Sends duplicate StartNextJobExecutionResponse to handler callback. Expect to only update and execute 1
     * JobExecution
     */
    const JobExecutionData job = getSampleJobExecution("job1", 1);
    startNextJobExecutionResponse->Execution = Aws::Crt::Optional<JobExecutionData>(job);

    // As JobEngine is run in a separate thread this is needed so that the tests wait for that thread to update JE
    std::promise<void> promise;
    auto setPromise = [&promise]() -> void { promise.set_value(); };

    string stdoutput = "test output";

    EXPECT_CALL(*jobsMock, createJobEngine()).Times(1).WillOnce(Return(mockEngine));
    EXPECT_CALL(*mockEngine, exec_steps(_, _)).WillOnce(Return(0));
    EXPECT_CALL(*mockEngine, hasErrors()).WillOnce(Return(1));
    EXPECT_CALL(*mockEngine, getReason(_)).WillOnce(Return(""));
    EXPECT_CALL(*mockEngine, getStdOut()).WillOnce(Return(stdoutput));
    EXPECT_CALL(*mockEngine, getStdErr()).WillOnce(Return(""));

    EXPECT_CALL(*jobsMock, createJobsClient()).Times(1).WillOnce(Return(mockClient));

    EXPECT_CALL(
        *mockClient,
        SubscribeToStartNextPendingJobExecutionAccepted(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(DoAll(
            InvokeArgument<3>(0),
            InvokeArgument<2>(startNextJobExecutionResponse.get(), 0),
            InvokeArgument<2>(startNextJobExecutionResponse.get(), 0)));

    EXPECT_CALL(
        *mockClient,
        SubscribeToStartNextPendingJobExecutionRejected(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(InvokeArgument<3>(0));
    EXPECT_CALL(
        *mockClient, SubscribeToNextJobExecutionChangedEvents(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(InvokeArgument<3>(0));
    EXPECT_CALL(
        *mockClient, SubscribeToUpdateJobExecutionAccepted(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(InvokeArgument<3>(0));
    EXPECT_CALL(
        *mockClient, SubscribeToUpdateJobExecutionRejected(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(InvokeArgument<3>(0));
    EXPECT_CALL(*mockClient, PublishStartNextPendingJobExecution(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _))
        .Times(1)
        .WillOnce(InvokeArgument<2>(0));

    EXPECT_CALL(
        *jobsMock,
        publishUpdateJobExecutionStatusWithRetry(
            JobExecutionEq(job),
            StatusInfoEq(JobsFeature::JobExecutionStatusInfo(Iotjobs::JobStatus::IN_PROGRESS, "", "", "")),
            IsEmpty(),
            IsNull()))
        .Times(1);
    EXPECT_CALL(
        *jobsMock,
        publishUpdateJobExecutionStatusWithRetry(
            JobExecutionEq(job),
            StatusInfoEq(JobsFeature::JobExecutionStatusInfo(Iotjobs::JobStatus::SUCCEEDED, "", stdoutput, "")),
            _,
            _))
        .WillOnce(InvokeWithoutArgs(setPromise));

    jobsMock->init(std::shared_ptr<Mqtt::MqttConnection>(), notifier, config);
    jobsMock->invokeRunJobs();

    EXPECT_EQ(std::future_status::ready, promise.get_future().wait_for(std::chrono::seconds(3)));
}

TEST_F(TestJobsFeature, InvalidJobDocument)
{
    /**
     * Invoke handler callback with invalid JobDocument, expect JobExecution rejected
     */
    JobExecutionData job;
    job.JobDocument = Aws::Crt::Optional<JsonObject>(JsonObject("{invalid}"));
    job.JobId = Aws::Crt::Optional<basic_string<char, char_traits<char>, StlAllocator<char>>>("invalid-job");
    job.ExecutionNumber = Aws::Crt::Optional<int64_t>(1);
    startNextJobExecutionResponse->Execution = Aws::Crt::Optional<JobExecutionData>(job);

    EXPECT_CALL(*jobsMock, createJobsClient()).Times(1).WillOnce(Return(mockClient));

    EXPECT_CALL(
        *mockClient,
        SubscribeToStartNextPendingJobExecutionAccepted(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(DoAll(InvokeArgument<3>(0), InvokeArgument<2>(startNextJobExecutionResponse.get(), 0)));
    EXPECT_CALL(
        *mockClient,
        SubscribeToStartNextPendingJobExecutionRejected(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(InvokeArgument<3>(0));
    EXPECT_CALL(
        *mockClient, SubscribeToNextJobExecutionChangedEvents(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(InvokeArgument<3>(0));
    EXPECT_CALL(
        *mockClient, SubscribeToUpdateJobExecutionAccepted(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(InvokeArgument<3>(0));
    EXPECT_CALL(
        *mockClient, SubscribeToUpdateJobExecutionRejected(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(InvokeArgument<3>(0));
    EXPECT_CALL(*mockClient, PublishStartNextPendingJobExecution(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _))
        .Times(1)
        .WillOnce(InvokeArgument<2>(0));

    EXPECT_CALL(
        *jobsMock,
        publishUpdateJobExecutionStatusWithRetry(
            JobExecutionEq(job),
            StatusInfoEq(JobsFeature::JobExecutionStatusInfo(
                Iotjobs::JobStatus::REJECTED, "Unable to execute job, invalid job document provided!", "", "")),
            _,
            _))
        .Times(1);

    jobsMock->init(std::shared_ptr<Mqtt::MqttConnection>(), notifier, config);
    jobsMock->invokeRunJobs();
}
