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

#include "aws/iotjobs/RejectedError.h"
#include "aws/iotjobs/StartNextJobExecutionResponse.h"
#include "aws/iotjobs/UpdateJobExecutionResponse.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

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

class MockJobsFeature : public JobsFeature
{
  public:
    MockJobsFeature() : JobsFeature() {}
    void invokeRunJobs() { this->runJobs(); }
    MOCK_METHOD(shared_ptr<AbstractIotJobsClient>, getJobsClient, (), (override));
    MOCK_METHOD(
        void,
        publishUpdateJobExecutionStatusWithRetry,
        (Aws::Iotjobs::JobExecutionData data,
         JobsFeature::JobExecutionStatusInfo statusInfo,
         (Aws::Crt::Map<Aws::Crt::String, Aws::Crt::String> statusDetails),
         std::function<void(void)> onCompleteCallback),
        (override));
    MOCK_METHOD(void, executeJob, (const JobExecutionData &job, const PlainJobDocument &jobDocument), (override));
};

class TestJobsFeature : public ::testing::Test
{
  public:
    void SetUp()
    {
        ThingName = Aws::Crt::String("thing-name value");
        notifier = shared_ptr<MockNotifier>(new MockNotifier());
        config = getSimpleConfig();
        jobsMock = unique_ptr<MockJobsFeature>(new MockJobsFeature());
        mockClient = shared_ptr<MockJobsClient>(new MockJobsClient());
    }
    Aws::Crt::String ThingName;
    shared_ptr<MockNotifier> notifier;
    PlainConfig config;
    unique_ptr<JobsFeature> jobs;
    unique_ptr<MockJobsFeature> jobsMock;
    shared_ptr<MockJobsClient> mockClient;
    StartNextJobExecutionResponse *startNextJobExecutionResponse;
};

MATCHER_P(ThingNameEq, ThingName, "Matcher ThingName for all Aws request Objects using Aws::Crt::String")
{
    return arg.ThingName.value() == ThingName;
}

MATCHER_P(StatusEq, status, "Matches JobExecutionStatusInfo status")
{
    return arg.status == status;
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
    EXPECT_CALL(*jobsMock, getJobsClient()).Times(1).WillOnce(Return(mockClient));

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
     * Inject a MockJobsClient into Jobs Feature and invoke RunJobs
     * Invokes the StartNextPendingJobExecution Handler with a Test JobExecution
     * Verifies Subscription Requests to IotJobsClient and invokes SubAck Callback functions
     * MockJobsFeature Mocks functions which start new threads so we simply verify their invocation here
     */
    const JobExecutionData job = getSampleJobExecution("job1", 1);
    startNextJobExecutionResponse = new StartNextJobExecutionResponse();
    startNextJobExecutionResponse->Execution = Aws::Crt::Optional<JobExecutionData>(job);

    EXPECT_CALL(*jobsMock, getJobsClient()).Times(1).WillOnce(Return(mockClient));

    EXPECT_CALL(
        *mockClient,
        SubscribeToStartNextPendingJobExecutionAccepted(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(DoAll(InvokeArgument<3>(0), InvokeArgument<2>(startNextJobExecutionResponse, 0)));
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
            JobExecutionEq(job), StatusEq(Iotjobs::JobStatus::IN_PROGRESS), IsEmpty(), IsNull()))
        .Times(1);
    EXPECT_CALL(*jobsMock, executeJob(JobExecutionEq(job), _)).Times(1);

    jobsMock->init(std::shared_ptr<Mqtt::MqttConnection>(), notifier, config);
    jobsMock->invokeRunJobs();

    delete startNextJobExecutionResponse;
}

TEST_F(TestJobsFeature, DuplicateJobNotification)
{
    /**
     * Sends duplicate StartNextJobExecutionResponse to handler callback. Expect to only update and execute 1
     * JobExecution
     */
    const JobExecutionData job = getSampleJobExecution("job1", 1);
    startNextJobExecutionResponse = new StartNextJobExecutionResponse();
    startNextJobExecutionResponse->Execution = Aws::Crt::Optional<JobExecutionData>(job);

    EXPECT_CALL(*jobsMock, getJobsClient()).Times(1).WillOnce(Return(mockClient));

    // Invokes handler twice with same Job
    EXPECT_CALL(
        *mockClient,
        SubscribeToStartNextPendingJobExecutionAccepted(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(DoAll(
            InvokeArgument<3>(0),
            InvokeArgument<2>(startNextJobExecutionResponse, 0),
            InvokeArgument<2>(startNextJobExecutionResponse, 0)));

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

    // Expects single update and execution
    EXPECT_CALL(
        *jobsMock,
        publishUpdateJobExecutionStatusWithRetry(
            JobExecutionEq(job), StatusEq(Iotjobs::JobStatus::IN_PROGRESS), IsEmpty(), IsNull()))
        .Times(1);
    EXPECT_CALL(*jobsMock, executeJob(JobExecutionEq(job), _)).Times(1);

    jobsMock->init(std::shared_ptr<Mqtt::MqttConnection>(), notifier, config);
    jobsMock->invokeRunJobs();

    delete startNextJobExecutionResponse;
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
    startNextJobExecutionResponse = new StartNextJobExecutionResponse();
    startNextJobExecutionResponse->Execution = Aws::Crt::Optional<JobExecutionData>(job);

    EXPECT_CALL(*jobsMock, getJobsClient()).Times(1).WillOnce(Return(mockClient));

    EXPECT_CALL(
        *mockClient,
        SubscribeToStartNextPendingJobExecutionAccepted(ThingNameEq(ThingName), AWS_MQTT_QOS_AT_LEAST_ONCE, _, _))
        .Times(1)
        .WillOnce(DoAll(InvokeArgument<3>(0), InvokeArgument<2>(startNextJobExecutionResponse, 0)));
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
        publishUpdateJobExecutionStatusWithRetry(JobExecutionEq(job), StatusEq(Iotjobs::JobStatus::REJECTED), _, _))
        .Times(1);

    jobsMock->init(std::shared_ptr<Mqtt::MqttConnection>(), notifier, config);
    jobsMock->invokeRunJobs();

    delete startNextJobExecutionResponse;
}
