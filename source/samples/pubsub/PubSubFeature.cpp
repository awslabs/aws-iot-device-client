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
#include <thread>
#include <unistd.h>
#include <utility>

#include <sys/inotify.h>

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
constexpr char PubSubFeature::NAME[];
constexpr char PubSubFeature::DEFAULT_PUBLISH_FILE[];
constexpr char PubSubFeature::DEFAULT_SUBSCRIBE_FILE[];

constexpr size_t MAX_IOT_CORE_MQTT_MESSAGE_SIZE_BYTES = 128000;
// Definitions for inode notify
constexpr size_t MAX_EVENTS = 1000; /* Maximum number of events to process */
constexpr size_t LEN_NAME = 16;     /* Assuming that the length of the filename won't exceed 16 bytes */
constexpr size_t EVENT_SIZE = (sizeof(struct inotify_event)); /* size of one event */
constexpr size_t EVENT_BUFSIZE =
    (MAX_EVENTS * (EVENT_SIZE + LEN_NAME)); /* size of buffer used to store the data of events */

const std::string PubSubFeature::DEFAULT_PUBLISH_PAYLOAD = R"({"Hello": "World!"})";
const std::string PubSubFeature::PUBLISH_TRIGGER_PAYLOAD = "DC-Publish";

string PubSubFeature::getName()
{
    return NAME;
}

bool PubSubFeature::createPubSub(const PlainConfig &config, const std::string &filePath, const aws_byte_buf *payload)
    const
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
        if (payload != nullptr)
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
    publishOnChange = config.pubSub.publishOnChange;

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

    if (!createPubSub(config, subFile, nullptr))
    {
        LOG_ERROR(TAG, "Failed to create subscribe directory or file");
    }

    return AWS_OP_SUCCESS;
}

void PubSubFeature::runFileMonitor()
{
    int len = 0;
    int fd = 0;
    int dir_wd = 0;
    int file_wd = 0;
    char buf[EVENT_BUFSIZE];
    fd = inotify_init();

    string fileDir = FileUtils::ExtractParentDirectory(pubFile.c_str());
    string fileName = pubFile.substr(fileDir.length());

    if (fd == -1)
    {
        LOGM_ERROR(TAG, "Encountered error %d while initializing the inode notify system return s%", fd);
        return;
    }

    dir_wd = inotify_add_watch(fd, fileDir.c_str(), IN_CREATE);
    if (dir_wd == -1)
    {
        LOGM_ERROR(TAG, "Encountered error %d while adding the watch for input file's parent directory", fd);
        goto exit;
    }

    file_wd = inotify_add_watch(fd, pubFile.c_str(), IN_CLOSE_WRITE);
    if (file_wd == -1)
    {
        LOGM_ERROR(TAG, "Encountered error %d while adding the watch for target file", fd);
        goto exit;
    }

    while (!needStop.load())
    {
        len = read(fd, buf, EVENT_BUFSIZE);
        if (len <= 0)
        {
            LOG_WARN(TAG, "Couldn't monitor any more target file modify events as it reaches max read buffer size");
            break;
        }

        for (int i = 0; i < len;)
        {
            struct inotify_event *e = (struct inotify_event *)&buf[i];

            if (e->mask & IN_CREATE && strcmp(e->name, fileName.c_str()) == 0 && !(e->mask & IN_ISDIR))
            {
                LOG_DEBUG(TAG, "New file is created with the same name of the target file.");
                publishFileData();
                file_wd = inotify_add_watch(fd, pubFile.c_str(), IN_CLOSE_WRITE);
            }
            else if (e->mask & IN_CLOSE_WRITE)
            {
                LOG_DEBUG(TAG, "The target file is modified, start updating the shadow");
                publishFileData();
            }

            i += EVENT_SIZE + e->len;
        }
        this_thread::sleep_for(std::chrono::milliseconds(500));
    }
exit:
    inotify_rm_watch(fd, file_wd);
    inotify_rm_watch(fd, dir_wd);
    close(fd);
}

int PubSubFeature::getPublishFileData(aws_byte_buf *buf) const
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
        aws_byte_buf_clean_up_secure(&payload);
        return;
    }
    auto onPublishComplete = [payload, this](const Mqtt::MqttConnection &, uint16_t, int errorCode) mutable {
        LOGM_DEBUG(TAG, "PublishCompAck: PacketId:(%s), ErrorCode:%d", getName().c_str(), errorCode);
        aws_byte_buf_clean_up_secure(&payload);
    };
    resourceManager->getConnection()->Publish(
        pubTopic.c_str(), AWS_MQTT_QOS_AT_LEAST_ONCE, false, payload, onPublishComplete);
}

int PubSubFeature::start()
{
    LOGM_INFO(TAG, "Starting %s", getName().c_str());

    auto onSubAck = [this](const MqttConnection &, uint16_t, const String &, QOS, int errorCode) -> void {
        LOGM_DEBUG(TAG, "SubAck: PacketId:(%s), ErrorCode:%d", getName().c_str(), errorCode);
    };
    auto onRecvData = [this](const MqttConnection &, const String &, const ByteBuf &payload) -> void {
        LOGM_DEBUG(TAG, "Message received on subscribe topic, size: %zu bytes", payload.len);
        if (string(reinterpret_cast<char *>(payload.buffer), payload.len) == PUBLISH_TRIGGER_PAYLOAD)
        {
            publishFileData();
        }
        if (subFile != "" && FileUtils::WriteToFile(subFile, &payload) != 0)
        {
            LOG_ERROR(TAG, "Failure writing incoming payload to subscribe file... Skipping");
        }
    };

    resourceManager->getConnection()->Subscribe(subTopic.c_str(), AWS_MQTT_QOS_AT_LEAST_ONCE, onRecvData, onSubAck);

    // The feature will always publish when starting up, and then will only republish if `PUBLISH_TRIGGER_PAYLOAD`
    // is received
    publishFileData();

    if (publishOnChange)
    {
        thread file_monitor_thread(&PubSubFeature::runFileMonitor, this);
        file_monitor_thread.detach();
    }

    baseNotifier->onEvent(static_cast<Feature *>(this), ClientBaseEventNotification::FEATURE_STARTED);
    return AWS_OP_SUCCESS;
}

int PubSubFeature::stop()
{
    needStop.store(true);

    auto onUnsubscribe = [](const MqttConnection &, uint16_t packetId, int errorCode) -> void {
        LOGM_DEBUG(TAG, "Unsubscribing: PacketId:%u, ErrorCode:%d", packetId, errorCode);
    };

    resourceManager->getConnection()->Unsubscribe(subTopic.c_str(), onUnsubscribe);
    baseNotifier->onEvent(static_cast<Feature *>(this), ClientBaseEventNotification::FEATURE_STOPPED);
    return AWS_OP_SUCCESS;
}