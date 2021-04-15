// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "PubSubFeature.h"
#include "../../logging/LoggerFactory.h"

#include <aws/common/byte_buf.h>
#include <aws/crt/Api.h>
#include <aws/iotdevicecommon/IotDevice.h>
#include <iostream>

#include <utility>

using namespace std;
using namespace Aws;
using namespace Aws::Iot;
using namespace Aws::Crt;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Crt::Mqtt;
using namespace Aws::Iot::DeviceClient::Samples;
using namespace Aws::Iot::DeviceClient::Util;
using namespace Aws::Iot::DeviceClient::Logging;

constexpr char PubSubFeature::TAG[];

#define MAX_IOT_CORE_MQTT_MESSAGE_SIZE_BYTES 128000

string PubSubFeature::getName()
{
    return "Pub Sub Sample";
}

int PubSubFeature::init(
    shared_ptr<SharedCrtResourceManager> manager,
    shared_ptr<ClientBaseNotifier> notifier,
    const PlainConfig &config)
{
    resourceManager = manager;
    baseNotifier = notifier;
    thingName = *config.thingName;
    pubTopic = config.pubSub.publishTopic.value();
    subTopic = config.pubSub.subscribeTopic.value();
    pubFile = config.pubSub.publishFile.has_value() ? config.pubSub.publishFile.value() : "";
    subFile = config.pubSub.subscribeFile.has_value() ? config.pubSub.subscribeFile.value() : "";

    return AWS_OP_SUCCESS;
}

int PubSubFeature::getPublishFileData(aws_byte_buf *buf)
{
    size_t publishFileSize = FileUtils::GetFileSize(pubFile);
    if (publishFileSize > MAX_IOT_CORE_MQTT_MESSAGE_SIZE_BYTES)
    {
        LOGM_ERROR(
            TAG, "Publish file too large: %zu > %i bytes", publishFileSize, MAX_IOT_CORE_MQTT_MESSAGE_SIZE_BYTES);
        return AWS_OP_ERR;
    }
    if (publishFileSize == 0)
    {
        LOG_ERROR(TAG, "Publish file contains no data");
        return AWS_OP_ERR;
    }

    aws_byte_buf_init(buf, resourceManager->getAllocator(), publishFileSize);
    if (FileUtils::ReadFromFile(pubFile, buf, publishFileSize) != 0)
    {
        LOG_ERROR(TAG, "Publish file data failed to read");
        return AWS_OP_ERR;
    }
    return AWS_OP_SUCCESS;
}

void PubSubFeature::publishFileData()
{
    ByteBuf payload;
    if (pubFile == "")
    {
        payload = ByteBufFromCString(DEFAULT_PUBLISH_PAYLOAD.c_str());
    }
    else if (getPublishFileData(&payload) != AWS_OP_SUCCESS)
    {
        LOG_ERROR(TAG, "Failed to read publish file... Skipping publish");
        return;
    }
    auto onPublishComplete = [payload, this](Mqtt::MqttConnection &, uint16_t packetId, int errorCode) mutable {
        LOGM_DEBUG(TAG, "PublishCompAck: PacketId:(%u), ErrorCode:%i", getName().c_str(), errorCode);
        aws_byte_buf_clean_up_secure(&payload);
    };
    resourceManager->getConnection()->Publish(
        pubTopic.c_str(), AWS_MQTT_QOS_AT_LEAST_ONCE, false, payload, onPublishComplete);
}

int PubSubFeature::start()
{
    LOGM_INFO(TAG, "Starting %s", getName().c_str());

    auto onSubAck =
        [&](MqttConnection &connection, uint16_t packetId, const String &topic, QOS qos, int errorCode) -> void {
        LOGM_DEBUG(TAG, "SubAck: PacketId:(%u), ErrorCode:%i", getName().c_str(), errorCode);
    };
    auto onRecvData = [&](MqttConnection &connection, const String &topic, const ByteBuf &payload) -> void {
        if (string((char *)payload.buffer) == PUBLISH_TRIGGER_PAYLOAD)
        {
            publishFileData();
        }
        if ((FileUtils::WriteToFile(subFile, &payload) != 0) && (subFile != ""))
        {
            LOG_ERROR(TAG, "Failure writing incoming payload to subscribe file... Skipping");
        }
    };

    resourceManager->getConnection()->Subscribe(subTopic.c_str(), AWS_MQTT_QOS_AT_LEAST_ONCE, onRecvData, onSubAck);

    // The feature will always publish when starting up, and then will only republish if `PUBLISH_TRIGGER_PAYLOAD` is
    // received
    publishFileData();

    baseNotifier->onEvent((Feature *)this, ClientBaseEventNotification::FEATURE_STARTED);
    return AWS_OP_SUCCESS;
}

int PubSubFeature::stop()
{
    auto onUnsubscribe = [&](MqttConnection &connection, uint16_t packetId, int errorCode) -> void {
        LOGM_DEBUG(TAG, "Unsubscribing: PacketId:%u, ErrorCode:%i", packetId, errorCode);
    };

    resourceManager->getConnection()->Unsubscribe(subTopic.c_str(), onUnsubscribe);
    baseNotifier->onEvent((Feature *)this, ClientBaseEventNotification::FEATURE_STOPPED);
    return AWS_OP_SUCCESS;
}