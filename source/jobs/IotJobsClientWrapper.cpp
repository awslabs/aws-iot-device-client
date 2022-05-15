//
// Created by Gable, Shane on 5/11/22.
//

#include "IotJobsClientWrapper.h"
#include "../../cmake-build-gg-st/aws-iot-device-sdk-cpp-v2-src/jobs/include/aws/iotjobs/UpdateJobExecutionRequest.h"
#include "JobsFeature.h"
#include "aws/iotjobs/NextJobExecutionChangedSubscriptionRequest.h"
#include "aws/iotjobs/StartNextPendingJobExecutionRequest.h"
#include "aws/iotjobs/StartNextPendingJobExecutionSubscriptionRequest.h"
#include "aws/iotjobs/UpdateJobExecutionSubscriptionRequest.h"

using namespace std;
using namespace Aws::Iot;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::Jobs;
using namespace Aws::Iotjobs;

IotJobsClientWrapper::IotJobsClientWrapper(
    const std::string &thingName,
    const std::shared_ptr<Aws::Crt::Mqtt::MqttConnection> connection)
    : thingName(thingName), mqttConnection(connection)
{
    jobsClient = std::unique_ptr<Aws::Iotjobs::IotJobsClient>(new IotJobsClient(mqttConnection));
}
void IotJobsClientWrapper::PublishStartNextPendingJobExecution(
    const StartNextPendingJobExecutionRequest &request,
    Aws::Crt::Mqtt::QOS qos,
    const OnPublishComplete &onPubAck)
{
    jobsClient->PublishStartNextPendingJobExecution(request, qos, onPubAck);
}
void IotJobsClientWrapper::SubscribeToStartNextPendingJobExecutionAccepted(
    const StartNextPendingJobExecutionSubscriptionRequest &request,
    Aws::Crt::Mqtt::QOS qos,
    const OnSubscribeToStartNextPendingJobExecutionAcceptedResponse &handler,
    const OnSubscribeComplete &onSubAck)
{
    jobsClient->SubscribeToStartNextPendingJobExecutionAccepted(request, qos, handler, onSubAck);
}
void IotJobsClientWrapper::SubscribeToStartNextPendingJobExecutionRejected(
    const StartNextPendingJobExecutionSubscriptionRequest &request,
    Aws::Crt::Mqtt::QOS qos,
    const OnSubscribeToStartNextPendingJobExecutionRejectedResponse &handler,
    const OnSubscribeComplete &onSubAck)
{
    jobsClient->SubscribeToStartNextPendingJobExecutionRejected(request, qos, handler, onSubAck);
}
void IotJobsClientWrapper::SubscribeToNextJobExecutionChangedEvents(
    const NextJobExecutionChangedSubscriptionRequest &request,
    Aws::Crt::Mqtt::QOS qos,
    const OnSubscribeToNextJobExecutionChangedEventsResponse &handler,
    const OnSubscribeComplete &onSubAck)
{
    jobsClient->SubscribeToNextJobExecutionChangedEvents(request, qos, handler, onSubAck);
}
void IotJobsClientWrapper::SubscribeToUpdateJobExecutionAccepted(
    const UpdateJobExecutionSubscriptionRequest &request,
    Aws::Crt::Mqtt::QOS qos,
    const OnSubscribeToUpdateJobExecutionAcceptedResponse &handler,
    const OnSubscribeComplete &onSubAck)
{
    jobsClient->SubscribeToUpdateJobExecutionAccepted(request, qos, handler, onSubAck);
}
void IotJobsClientWrapper::SubscribeToUpdateJobExecutionRejected(
    const UpdateJobExecutionSubscriptionRequest &request,
    Aws::Crt::Mqtt::QOS qos,
    const OnSubscribeToUpdateJobExecutionRejectedResponse &handler,
    const OnSubscribeComplete &onSubAck)
{
    jobsClient->SubscribeToUpdateJobExecutionRejected(request, qos, handler, onSubAck);
}
void IotJobsClientWrapper::PublishUpdateJobExecution(
    const UpdateJobExecutionRequest &request,
    Aws::Crt::Mqtt::QOS qos,
    const OnPublishComplete &onPubAck)
{
    jobsClient->PublishUpdateJobExecution(request, qos, onPubAck);
}