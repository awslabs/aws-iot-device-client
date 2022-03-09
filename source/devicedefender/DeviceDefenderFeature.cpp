// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "DeviceDefenderFeature.h"
#include "../logging/LoggerFactory.h"
#include <aws/iotdevicedefender/DeviceDefender.h>

#include <aws/crt/Api.h>
#include <aws/iotdevicecommon/IotDevice.h>
#include <iostream>
#include <thread>

#include <aws/iotdevicecommon/IotDevice.h>
#include <aws/iotdevicedefender/DeviceDefender.h>
#include <utility>

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
constexpr char DeviceDefenderFeature::TOPIC_PRE[];
constexpr char DeviceDefenderFeature::TOPIC_POST[];
constexpr char DeviceDefenderFeature::TOPIC_ACCEPTED[];
constexpr char DeviceDefenderFeature::TOPIC_REJECTED[];

string DeviceDefenderFeature::getName()
{
    return "Device Defender";
}

void DeviceDefenderFeature::startDeviceDefender()
{
    LOGM_INFO(TAG, "Starting %s", getName().c_str());

    auto onCancelled = [&](void *userData) -> void {
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
    taskBuilder.WithTaskCancellationUserData(NULL);
    LOGM_INFO(TAG, "%s task builder interval: %i", getName().c_str(), interval);
    task = taskBuilder.Build();
    LOGM_DEBUG(TAG, "%s task build finished", getName().c_str());
    task->StartTask();
    LOGM_DEBUG(TAG, "%s StartTask() async called", getName().c_str());

    auto onRecvData = [&](MqttConnection &connection, const String &topic, const ByteBuf &payload) -> void {
        LOGM_DEBUG(TAG, "Recv: Topic:(%s), Payload:%s", topic.c_str(), (char *)payload.buffer);
    };
    auto onSubAck =
        [&](MqttConnection &connection, uint16_t packetId, const String &topic, QOS qos, int errorCode) -> void {
        LOGM_DEBUG(TAG, "SubAck: PacketId:(%s), ErrorCode:%i", getName().c_str(), errorCode);
    };
    const char *messageFormat = "%s%s%s%s";
    resourceManager->getConnection()->Subscribe(
        FormatMessage(messageFormat, TOPIC_PRE, thingName.c_str(), TOPIC_POST, TOPIC_ACCEPTED).c_str(),
        AWS_MQTT_QOS_AT_LEAST_ONCE,
        onRecvData,
        onSubAck);
    resourceManager->getConnection()->Subscribe(
        FormatMessage(messageFormat, TOPIC_PRE, thingName.c_str(), TOPIC_POST, TOPIC_REJECTED).c_str(),
        AWS_MQTT_QOS_AT_LEAST_ONCE,
        onRecvData,
        onSubAck);
}

void DeviceDefenderFeature::stopDeviceDefender()
{
    LOGM_INFO(TAG, "Stopping %s", getName().c_str());
    task->StopTask();
    auto onUnsubscribe = [&](MqttConnection &connection, uint16_t packetId, int errorCode) -> void {
        LOGM_DEBUG(TAG, "Unsubscribing: PacketId:%i, ErrorCode:%i", packetId, errorCode);
    };
    const char *messageFormat = "%s%s%s%s";
    resourceManager->getConnection()->Unsubscribe(
        FormatMessage(messageFormat, TOPIC_PRE, thingName.c_str(), TOPIC_POST, TOPIC_ACCEPTED).c_str(), onUnsubscribe);
    resourceManager->getConnection()->Unsubscribe(
        FormatMessage(messageFormat, TOPIC_PRE, thingName.c_str(), TOPIC_POST, TOPIC_REJECTED).c_str(), onUnsubscribe);
    LOGM_DEBUG(TAG, "%s StopTask() async called", getName().c_str());
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
    baseNotifier->onEvent((Feature *)this, ClientBaseEventNotification::FEATURE_STARTED);
    return 0;
}

int DeviceDefenderFeature::stop()
{
    stopDeviceDefender();
    baseNotifier->onEvent((Feature *)this, ClientBaseEventNotification::FEATURE_STOPPED);
    return 0;
}
