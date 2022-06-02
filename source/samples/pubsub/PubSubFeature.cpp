// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "PubSubFeature.h"
#include "../../logging/LoggerFactory.h"
#include "../../util/FileUtils.h"

#include <aws/common/byte_buf.h>
#include <aws/crt/Api.h>
#include <aws/iotdevicecommon/IotDevice.h>
#include <iostream>
#include <sys/stat.h>

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
constexpr char PubSubFeature::DEFAULT_PUBLISH_FILE[];
constexpr char PubSubFeature::DEFAULT_SUBSCRIBE_FILE[];

#define MAX_IOT_CORE_MQTT_MESSAGE_SIZE_BYTES 128000

const std::string PubSubFeature::DEFAULT_PUBLISH_PAYLOAD = R"({"Hello": "World!"})";
const std::string PubSubFeature::PUBLISH_TRIGGER_PAYLOAD = "DC-Publish";

string PubSubFeature::getName()
{
    return "Pub Sub Sample";
}

bool PubSubFeature::createPubSub(const PlainConfig &config, std::string filePath, const aws_byte_buf *payload)
{
    std::string pubSubFileDir = FileUtils::ExtractParentDirectory(filePath);
    LOGM_INFO(TAG, "Creating Pub/Sub file: %s", filePath.c_str());
    if (!FileUtils::DirectoryExists(pubSubFileDir))
    {
        // Create an empty directory with the expected permissions.
        if (!FileUtils::CreateDirectoryWithPermissions(pubSubFileDir.c_str(), S_IRWXU | S_IRGRP | S_IROTH | S_IXOTH))
        {
            return false;
        }
    }
    else
    {
        // Verify the directory permissions.
        auto rcvDirPermissions = FileUtils::GetFilePermissions(pubSubFileDir);
        if (Permissions::PUBSUB_DIR != rcvDirPermissions)
        {
            LOGM_ERROR(
                TAG,
                "Incorrect directory permissions for pubsub file: %s expected: %d received: %d",
                Sanitize(pubSubFileDir).c_str(),
                Permissions::PUBSUB_DIR,
                rcvDirPermissions);
            return false;
        }
    }

    if (!FileUtils::FileExists(filePath))
    {
        // Create an empty file with the expected permissions.
        if (!FileUtils::CreateEmptyFileWithPermissions(filePath, S_IRUSR | S_IWUSR))
        {
            return false;
        }
        // Write payload data in newly created empty file.
        if (payload != NULL)
        {
            FileUtils::WriteToFile(filePath, payload);
        }
    }
    else
    {
        // Verify the file permissions.
        auto rcvFilePermissions = FileUtils::GetFilePermissions(filePath);
        if (Permissions::PUB_SUB_FILES != rcvFilePermissions)
        {
            LOGM_ERROR(
                TAG,
                "Incorrect file permissions for pubsub file: %s expected: %d received: %d",
                Sanitize(filePath).c_str(),
                Permissions::PUB_SUB_FILES,
                rcvFilePermissions);
            return false;
        }
    }

    return true;
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

    if (config.pubSub.publishFile.has_value() && !config.pubSub.publishFile->empty())
    {
        pubFile = config.pubSub.publishFile->c_str();
    }
    pubFile = FileUtils::ExtractExpandedPath(pubFile);

    ByteBuf payload;
    payload = aws_byte_buf_from_c_str(DEFAULT_PUBLISH_PAYLOAD.c_str());

    if (!createPubSub(config, pubFile, &payload))
    {
        LOG_ERROR(TAG, "Failed to create publish directory or file");
    }

    if (config.pubSub.subscribeFile.has_value() && !config.pubSub.subscribeFile->empty())
    {
        subFile = config.pubSub.subscribeFile->c_str();
    }
    subFile = FileUtils::ExtractExpandedPath(subFile);

    if (!createPubSub(config, subFile, NULL))
    {
        LOG_ERROR(TAG, "Failed to create subscribe directory or file");
    }

    return AWS_OP_SUCCESS;
}

int PubSubFeature::getPublishFileData(aws_byte_buf *buf)
{
    size_t publishFileSize = FileUtils::GetFileSize(pubFile);
    if (publishFileSize > MAX_IOT_CORE_MQTT_MESSAGE_SIZE_BYTES)
    {
        LOGM_ERROR(
            TAG, "Publish file too large: %zu > %d bytes", publishFileSize, MAX_IOT_CORE_MQTT_MESSAGE_SIZE_BYTES);
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

    if (getPublishFileData(&payload) != AWS_OP_SUCCESS)
    {
        LOG_ERROR(TAG, "Failed to read publish file... Skipping publish");
        return;
    }
    auto onPublishComplete = [payload, this](Mqtt::MqttConnection &, uint16_t packetId, int errorCode) mutable {
        LOGM_DEBUG(TAG, "PublishCompAck: PacketId:(%s), ErrorCode:%d", getName().c_str(), errorCode);
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
        LOGM_DEBUG(TAG, "SubAck: PacketId:(%s), ErrorCode:%d", getName().c_str(), errorCode);
    };
    auto onRecvData = [&](MqttConnection &connection, const String &topic, const ByteBuf &payload) -> void {
        LOGM_DEBUG(TAG, "Message received on subscribe topic, size: %zu bytes", payload.len);
        if (string((char *)payload.buffer, payload.len) == PUBLISH_TRIGGER_PAYLOAD)
        {
            publishFileData();
        }
        if (subFile != "")
        {
            if (FileUtils::WriteToFile(subFile, &payload) != 0)
            {
                LOG_ERROR(TAG, "Failure writing incoming payload to subscribe file... Skipping");
            }
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
        LOGM_DEBUG(TAG, "Unsubscribing: PacketId:%u, ErrorCode:%d", packetId, errorCode);
    };

    resourceManager->getConnection()->Unsubscribe(subTopic.c_str(), onUnsubscribe);
    baseNotifier->onEvent((Feature *)this, ClientBaseEventNotification::FEATURE_STOPPED);
    return AWS_OP_SUCCESS;
}