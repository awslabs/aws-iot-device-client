// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "DeviceDefenderFeature.h"
#include "../logging/LoggerFactory.h"
#include "ReportTaskWrapper.h"

#include <aws/iotdevicedefender/DeviceDefender.h>

#include <aws/iotdevicecommon/IotDevice.h>
#include <iostream>
#include <thread>

using namespace std;
using namespace Aws;
using namespace Aws::Iot;
using namespace Aws::Crt;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Crt::Mqtt;
using namespace Aws::Iot::DeviceClient::DeviceDefender;
using namespace Aws::Iot::DeviceClient::Util;
using namespace Aws::Iot::DeviceClient::Logging;

constexpr char DeviceDefenderFeature::TAG[];
constexpr char DeviceDefenderFeature::NAME[];
constexpr char DeviceDefenderFeature::TOPIC_PRE[];
constexpr char DeviceDefenderFeature::TOPIC_POST[];
constexpr char DeviceDefenderFeature::TOPIC_ACCEPTED[];
constexpr char DeviceDefenderFeature::TOPIC_REJECTED[];
constexpr char DeviceDefenderFeature::TOPIC_FORMAT[];

DeviceDefenderFeature::DeviceDefenderFeature() : interval(300) {}

string DeviceDefenderFeature::getName()
{
    return NAME;
}

void DeviceDefenderFeature::startDeviceDefender()
{
    LOGM_INFO(TAG, "Starting %s", getName().c_str());
    task = createReportTask();
    LOGM_DEBUG(TAG, "%s task build finished", getName().c_str());
    task->StartTask();
    LOGM_DEBUG(TAG, "%s StartTask() async called", getName().c_str());

    subscribeToTopicFilter();
}

void DeviceDefenderFeature::stopDeviceDefender()
{

    LOGM_INFO(TAG, "Stopping %s", getName().c_str());
    task->StopTask();
    unsubscribeToTopicFilter();
}

int DeviceDefenderFeature::init(
    shared_ptr<SharedCrtResourceManager> manager,
    shared_ptr<ClientBaseNotifier> notifier,
    const PlainConfig &config)
{
    resourceManager = manager;
    baseNotifier = notifier;
    interval = config.deviceDefender.interval;
    thingName = *config.thingName;
    return 0;
}

int DeviceDefenderFeature::start()
{
    startDeviceDefender();
    baseNotifier->onEvent(static_cast<Feature *>(this), ClientBaseEventNotification::FEATURE_STARTED);
    return 0;
}

int DeviceDefenderFeature::stop()
{
    stopDeviceDefender();
    baseNotifier->onEvent(static_cast<Feature *>(this), ClientBaseEventNotification::FEATURE_STOPPED);
    return 0;
}
std::shared_ptr<AbstractReportTask> DeviceDefender::DeviceDefenderFeature::createReportTask()
{

    auto onCancelled = [this](void *userData) -> void {
        LOGM_DEBUG(TAG, "task called onCancelled for thing: %s", thingName.c_str());
        stop();
    };

    Iotdevicedefenderv1::ReportTaskBuilder taskBuilder(
        resourceManager->getAllocator(),
        resourceManager->getConnection(),
        *resourceManager->getEventLoopGroup(),
        String(thingName.c_str()));
    taskBuilder.WithTaskPeriodSeconds((uint32_t)interval);
    taskBuilder.WithNetworkConnectionSamplePeriodSeconds((uint32_t)interval);
    taskBuilder.WithTaskCancelledHandler(onCancelled);
    taskBuilder.WithTaskCancellationUserData(nullptr);

    LOGM_INFO(TAG, "%s task builder interval: %i", getName().c_str(), interval);

    std::shared_ptr<Aws::Iotdevicedefenderv1::ReportTask> reportTask = taskBuilder.Build();

    return std::make_shared<ReportTaskWrapper>(reportTask);
}
void DeviceDefender::DeviceDefenderFeature::subscribeToTopicFilter()
{
    auto onRecvData = [](const MqttConnection &, const String &topic, const ByteBuf &payload) -> void {
        LOGM_DEBUG(TAG, "Recv: Topic:(%s), Payload:%s", topic.c_str(), reinterpret_cast<char *>(payload.buffer));
    };
    auto onSubAck = [this](const MqttConnection &, uint16_t, const String &, QOS, int errorCode) -> void {
        LOGM_DEBUG(TAG, "SubAck: PacketId:(%s), ErrorCode:%i", getName().c_str(), errorCode);
    };
    resourceManager->getConnection()->Subscribe(
        FormatMessage(TOPIC_FORMAT, TOPIC_PRE, thingName.c_str(), TOPIC_POST, TOPIC_ACCEPTED).c_str(),
        AWS_MQTT_QOS_AT_LEAST_ONCE,
        onRecvData,
        onSubAck);
    resourceManager->getConnection()->Subscribe(
        FormatMessage(TOPIC_FORMAT, TOPIC_PRE, thingName.c_str(), TOPIC_POST, TOPIC_REJECTED).c_str(),
        AWS_MQTT_QOS_AT_LEAST_ONCE,
        onRecvData,
        onSubAck);
}
void DeviceDefender::DeviceDefenderFeature::unsubscribeToTopicFilter()
{
    auto onUnsubscribe = [](const MqttConnection &, uint16_t packetId, int errorCode) -> void {
        LOGM_DEBUG(TAG, "Unsubscribing: PacketId:%i, ErrorCode:%i", packetId, errorCode);
    };
    resourceManager->getConnection()->Unsubscribe(
        FormatMessage(TOPIC_FORMAT, TOPIC_PRE, thingName.c_str(), TOPIC_POST, TOPIC_ACCEPTED).c_str(), onUnsubscribe);
    resourceManager->getConnection()->Unsubscribe(
        FormatMessage(TOPIC_FORMAT, TOPIC_PRE, thingName.c_str(), TOPIC_POST, TOPIC_REJECTED).c_str(), onUnsubscribe);
    LOGM_DEBUG(TAG, "%s StopTask() async called", getName().c_str());
}
