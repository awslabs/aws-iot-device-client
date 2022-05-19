// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef AWS_IOT_DEVICE_CLIENT_IOTJOBSCLIENTWRAPPER_H
#define AWS_IOT_DEVICE_CLIENT_IOTJOBSCLIENTWRAPPER_H

#include "../SharedCrtResourceManager.h"
#include "JobsFeature.h"
#include <aws/iotjobs/IotJobsClient.h>
#include <aws/iotjobs/JobStatus.h>
#include <memory>
#include <string>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Jobs
            {
                class AbstractIotJobsClient
                {
                  public:
                    virtual ~AbstractIotJobsClient() = default;
                    virtual void PublishStartNextPendingJobExecution(
                        const Iotjobs::StartNextPendingJobExecutionRequest &request,
                        Aws::Crt::Mqtt::QOS qos,
                        const Iotjobs::OnPublishComplete &onPubAck) = 0;

                    virtual void SubscribeToStartNextPendingJobExecutionAccepted(
                        const Iotjobs::StartNextPendingJobExecutionSubscriptionRequest &request,
                        Aws::Crt::Mqtt::QOS qos,
                        const Iotjobs::OnSubscribeToStartNextPendingJobExecutionAcceptedResponse &handler,
                        const Iotjobs::OnSubscribeComplete &onSubAck) = 0;

                    virtual void SubscribeToStartNextPendingJobExecutionRejected(
                        const Iotjobs::StartNextPendingJobExecutionSubscriptionRequest &request,
                        Aws::Crt::Mqtt::QOS qos,
                        const Iotjobs::OnSubscribeToStartNextPendingJobExecutionRejectedResponse &handler,
                        const Iotjobs::OnSubscribeComplete &onSubAck) = 0;

                    virtual void SubscribeToNextJobExecutionChangedEvents(
                        const Iotjobs::NextJobExecutionChangedSubscriptionRequest &request,
                        Aws::Crt::Mqtt::QOS qos,
                        const Iotjobs::OnSubscribeToNextJobExecutionChangedEventsResponse &handler,
                        const Iotjobs::OnSubscribeComplete &onSubAck) = 0;

                    virtual void SubscribeToUpdateJobExecutionAccepted(
                        const Iotjobs::UpdateJobExecutionSubscriptionRequest &request,
                        Aws::Crt::Mqtt::QOS qos,
                        const Iotjobs::OnSubscribeToUpdateJobExecutionAcceptedResponse &handler,
                        const Iotjobs::OnSubscribeComplete &onSubAck) = 0;

                    virtual void SubscribeToUpdateJobExecutionRejected(
                        const Iotjobs::UpdateJobExecutionSubscriptionRequest &request,
                        Aws::Crt::Mqtt::QOS qos,
                        const Iotjobs::OnSubscribeToUpdateJobExecutionRejectedResponse &handler,
                        const Iotjobs::OnSubscribeComplete &onSubAck) = 0;

                    virtual void PublishUpdateJobExecution(
                        const Iotjobs::UpdateJobExecutionRequest &request,
                        Aws::Crt::Mqtt::QOS qos,
                        const Iotjobs::OnPublishComplete &onPubAck) = 0;
                };

                class IotJobsClientWrapper : public AbstractIotJobsClient
                {
                  public:
                    explicit IotJobsClientWrapper(std::shared_ptr<Aws::Crt::Mqtt::MqttConnection> connection);
                    void PublishStartNextPendingJobExecution(
                        const Iotjobs::StartNextPendingJobExecutionRequest &request,
                        Aws::Crt::Mqtt::QOS qos,
                        const Iotjobs::OnPublishComplete &onPubAck) override;

                    void SubscribeToStartNextPendingJobExecutionAccepted(
                        const Iotjobs::StartNextPendingJobExecutionSubscriptionRequest &request,
                        Aws::Crt::Mqtt::QOS qos,
                        const Iotjobs::OnSubscribeToStartNextPendingJobExecutionAcceptedResponse &handler,
                        const Iotjobs::OnSubscribeComplete &onSubAck) override;

                    void SubscribeToStartNextPendingJobExecutionRejected(
                        const Iotjobs::StartNextPendingJobExecutionSubscriptionRequest &request,
                        Aws::Crt::Mqtt::QOS qos,
                        const Iotjobs::OnSubscribeToStartNextPendingJobExecutionRejectedResponse &handler,
                        const Iotjobs::OnSubscribeComplete &onSubAck) override;

                    void SubscribeToNextJobExecutionChangedEvents(
                        const Iotjobs::NextJobExecutionChangedSubscriptionRequest &request,
                        Aws::Crt::Mqtt::QOS qos,
                        const Iotjobs::OnSubscribeToNextJobExecutionChangedEventsResponse &handler,
                        const Iotjobs::OnSubscribeComplete &onSubAck) override;

                    void SubscribeToUpdateJobExecutionAccepted(
                        const Iotjobs::UpdateJobExecutionSubscriptionRequest &request,
                        Aws::Crt::Mqtt::QOS qos,
                        const Iotjobs::OnSubscribeToUpdateJobExecutionAcceptedResponse &handler,
                        const Iotjobs::OnSubscribeComplete &onSubAck) override;

                    void SubscribeToUpdateJobExecutionRejected(
                        const Iotjobs::UpdateJobExecutionSubscriptionRequest &request,
                        Aws::Crt::Mqtt::QOS qos,
                        const Iotjobs::OnSubscribeToUpdateJobExecutionRejectedResponse &handler,
                        const Iotjobs::OnSubscribeComplete &onSubAck) override;

                    void PublishUpdateJobExecution(
                        const Iotjobs::UpdateJobExecutionRequest &request,
                        Aws::Crt::Mqtt::QOS qos,
                        const Iotjobs::OnPublishComplete &onPubAck) override;

                  private:
                    std::unique_ptr<Aws::Iotjobs::IotJobsClient> jobsClient;
                };
            } // namespace Jobs
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // AWS_IOT_DEVICE_CLIENT_IOTJOBSCLIENTWRAPPER_H
