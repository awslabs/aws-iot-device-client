// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "SampleShadowFeature.h"
#include "../logging/LoggerFactory.h"
#include <aws/common/byte_buf.h>
#include <aws/crt/UUID.h>
#include <aws/iotdevicecommon/IotDevice.h>
#include <chrono>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <utility>

#include <aws/iotshadow/ErrorResponse.h>
#include <aws/iotshadow/IotShadowClient.h>
#include <aws/iotshadow/NamedShadowDeltaUpdatedSubscriptionRequest.h>
#include <aws/iotshadow/NamedShadowUpdatedSubscriptionRequest.h>
#include <aws/iotshadow/ShadowDeltaUpdatedEvent.h>
#include <aws/iotshadow/ShadowUpdatedEvent.h>
#include <aws/iotshadow/UpdateNamedShadowRequest.h>
#include <aws/iotshadow/UpdateNamedShadowSubscriptionRequest.h>
#include <aws/iotshadow/UpdateShadowRequest.h>
#include <aws/iotshadow/UpdateShadowResponse.h>

#include <sys/inotify.h>

using namespace std;
using namespace Aws;
using namespace Aws::Iot;
using namespace Aws::Crt;
using namespace Aws::Iotshadow;
using namespace Aws::Iot::DeviceClient::Shadow;
using namespace Aws::Iot::DeviceClient::Util;
using namespace Aws::Iot::DeviceClient::Logging;

constexpr char SampleShadowFeature::TAG[];
constexpr char SampleShadowFeature::NAME[];
constexpr char SampleShadowFeature::DEFAULT_SAMPLE_SHADOW_DOCUMENT_FILE[];
constexpr int SampleShadowFeature::DEFAULT_WAIT_TIME_SECONDS;

constexpr int MAX_EVENTS = 1000;                  /* Maximum number of events to process*/
constexpr int LEN_NAME = 16;                      /* Assuming that the length of the filename won't exceed 16 bytes*/
#define EVENT_SIZE (sizeof(struct inotify_event)) /*size of one event*/
#define EVENT_BUFSIZE (MAX_EVENTS * (EVENT_SIZE + LEN_NAME)) /*size of buffer used to store the data of events*/

string SampleShadowFeature::getName()
{
    return NAME;
}

int SampleShadowFeature::init(
    shared_ptr<SharedCrtResourceManager> manager,
    shared_ptr<ClientBaseNotifier> notifier,
    const PlainConfig &config)
{
    resourceManager = manager;
    baseNotifier = notifier;
    thingName = *config.thingName;
    shadowName = config.sampleShadow.shadowName.value();
    if (config.sampleShadow.shadowInputFile.has_value())
    {
        inputFile = config.sampleShadow.shadowInputFile.value();
    }

    if (config.sampleShadow.shadowOutputFile.has_value())
    {
        outputFile = config.sampleShadow.shadowOutputFile.value();
    }

    return AWS_OP_SUCCESS;
}

void SampleShadowFeature::updateNamedShadowAcceptedHandler(Iotshadow::UpdateShadowResponse *response, int ioError) const
{
    if (ioError)
    {
        LOGM_ERROR(TAG, "Encountered ioError %d within updateNamedShadowAcceptedHandler", ioError);
    }
}

void SampleShadowFeature::updateNamedShadowRejectedHandler(Iotshadow::ErrorResponse *errorResponse, int ioError) const
{
    if (ioError)
    {
        LOGM_ERROR(TAG, "Encountered ioError %d within updateNamedShadowRejectedHandler", ioError);
        return;
    }

    if (errorResponse->Message.has_value())
    {
        LOGM_ERROR(TAG, "UpdateNamedShadowRequest gets rejected: %s", errorResponse->Message->c_str());
    }
}

void SampleShadowFeature::updateNamedShadowEventHandler(Iotshadow::ShadowUpdatedEvent *shadowUpdatedEvent, int ioError)
    const
{
    if (ioError)
    {
        LOGM_ERROR(TAG, "Encountered ioError %d within updateNamedShadowEventHandler", ioError);
        return;
    }

    // write the response to output file
    Crt::JsonObject object;
    shadowUpdatedEvent->SerializeToObject(object);
    if (FileUtils::StoreValueInFile(object.View().WriteReadable(true).c_str(), outputFile))
    {
        LOGM_INFO(TAG, "Stored the latest %s shadow document to local successfully", shadowName.c_str());
    }
    else
    {
        LOGM_ERROR(TAG, "Failed to store latest %s shadow document to local", shadowName.c_str());
    }
}

void SampleShadowFeature::updateNamedShadowDeltaHandler(
    Iotshadow::ShadowDeltaUpdatedEvent *shadowDeltaUpdatedEvent,
    int ioError)
{
    if (ioError)
    {
        LOGM_ERROR(TAG, "Encountered ioError %d within updateNamedShadowDeltaHandler", ioError);
        return;
    }

    // do the shadow sync
    UpdateNamedShadowRequest updateNamedShadowRequest;
    updateNamedShadowRequest.ThingName = thingName.c_str();
    updateNamedShadowRequest.ShadowName = shadowName.c_str();
    ShadowState state;
    state.Reported = shadowDeltaUpdatedEvent->State.value();
    updateNamedShadowRequest.State = state;
    Aws::Crt::UUID uuid;
    updateNamedShadowRequest.ClientToken = uuid.ToString();

    shadowClient->PublishUpdateNamedShadow(
        updateNamedShadowRequest,
        AWS_MQTT_QOS_AT_LEAST_ONCE,
        std::bind(&SampleShadowFeature::ackUpdateNamedShadowStatus, this, std::placeholders::_1));
}

void SampleShadowFeature::ackUpdateNamedShadowStatus(int ioError) const
{
    LOGM_DEBUG(TAG, "Ack received for updateNamedShadowStatus with code {%d}", ioError);
}

void SampleShadowFeature::ackSubscribeToUpdateNamedShadowAccepted(int ioError)
{
    LOGM_DEBUG(TAG, "Ack received for SubscribeToUpdateNamedShadowAccepted with code {%d}", ioError);
    if (ioError)
    {
        string errorMessage = "Encountered an ioError while attempting to subscribe to UpdateNamedShadowAccepted";
        LOG_ERROR(TAG, errorMessage.c_str());
        baseNotifier->onError(this, ClientBaseErrorNotification::SUBSCRIPTION_FAILED, errorMessage);
    }
    subscribeShadowUpdateAcceptedPromise.set_value(ioError == AWS_OP_SUCCESS);
}

void SampleShadowFeature::ackSubscribeToUpdateNamedShadowRejected(int ioError)
{
    LOGM_DEBUG(TAG, "Ack received for SubscribeToUpdateNamedShadowRejected with code {%d}", ioError);
    if (ioError)
    {
        string errorMessage = "Encountered an ioError while attempting to subscribe to UpdateNamedShadowRejected";
        LOG_ERROR(TAG, errorMessage.c_str());
        baseNotifier->onError(this, ClientBaseErrorNotification::SUBSCRIPTION_FAILED, errorMessage);
    }
    subscribeShadowUpdateRejectedPromise.set_value(ioError == AWS_OP_SUCCESS);
}

void SampleShadowFeature::ackSubscribeToUpdateEvent(int ioError)
{
    LOGM_DEBUG(TAG, "Ack received for SubscribeToUpdateNamedShadowEvent with code {%d}", ioError);
    if (ioError)
    {
        string errorMessage = "Encountered an ioError while attempting to subscribe to UpdateNamedShadowEvent";
        LOG_ERROR(TAG, errorMessage.c_str());
        baseNotifier->onError(this, ClientBaseErrorNotification::SUBSCRIPTION_FAILED, errorMessage);
    }
    subscribeShadowUpdateEventPromise.set_value(ioError == AWS_OP_SUCCESS);
}

void SampleShadowFeature::ackSubscribeToUpdateDelta(int ioError)
{
    LOGM_DEBUG(TAG, "Ack received for SubscribeToUpdateNamedShadowDelta with code {%d}", ioError);
    if (ioError)
    {
        string errorMessage = "Encountered an ioError while attempting to subscribe to UpdateNamedShadowDelta";
        LOG_ERROR(TAG, errorMessage.c_str());
        baseNotifier->onError(this, ClientBaseErrorNotification::SUBSCRIPTION_FAILED, errorMessage);
    }
    subscribeShadowUpdateDeltaPromise.set_value(ioError == AWS_OP_SUCCESS);
}

void SampleShadowFeature::runFileMonitor()
{
    ssize_t len = 0;
    string fileDir = FileUtils::ExtractParentDirectory(inputFile.c_str());
    string fileName = inputFile.substr(fileDir.length());
    char buf[EVENT_BUFSIZE];

    int fd = 0;
    int dir_wd = 0;
    int file_wd = 0;

    fd = inotify_init();

    if (fd == -1)
    {
        LOGM_ERROR(TAG, "Encounter error %d while initializing the inode notify system return s%", fd);
        return;
    }

    dir_wd = inotify_add_watch(fd, fileDir.c_str(), IN_CREATE);
    if (dir_wd == -1)
    {
        LOGM_ERROR(TAG, "Encounter error %d while adding the watch for input file's parent directory", fd);
        goto exit;
    }

    file_wd = inotify_add_watch(fd, inputFile.c_str(), IN_CLOSE_WRITE);
    if (file_wd == -1)
    {
        LOGM_ERROR(TAG, "Encounter error %d while adding the watch for target file", fd);
        goto exit;
    }

    while (!needStop.load())
    {
        len = read(fd, buf, EVENT_BUFSIZE);
        if (len <= 0)
        {
            LOG_WARN(TAG, "Couldn't monitor any more target file modify events as it reaches max read buffer size");
            goto exit;
        }

        for (int i = 0; i < len;)
        {
            auto *e = (struct inotify_event *)&buf[i];

            if (e->mask & IN_CREATE)
            {
                if (strcmp(e->name, fileName.c_str()) != 0)
                    goto next;

                if (e->mask & IN_ISDIR)
                    goto next;

                LOG_DEBUG(TAG, "New file is created with the same name of target file, start updating the shadow");
                readAndUpdateShadowFromFile();
                file_wd = inotify_add_watch(fd, inputFile.c_str(), IN_CLOSE_WRITE | IN_DELETE_SELF);
            }

            if (e->mask & IN_CLOSE_WRITE)
            {
                LOG_DEBUG(TAG, "The target file is modified, start updating the shadow");
                readAndUpdateShadowFromFile();
            }

            if (e->mask & IN_DELETE_SELF)
            {
                if (e->mask & IN_ISDIR)
                    goto next;

                LOG_DEBUG(TAG, "The target file is deleted by itself, removing the watch");
                inotify_rm_watch(fd, file_wd);
            }

        next:
            i += EVENT_SIZE + e->len;
        }

        this_thread::sleep_for(std::chrono::milliseconds(500));
    }

exit:
    inotify_rm_watch(fd, file_wd);
    inotify_rm_watch(fd, dir_wd);
    close(fd);
}

bool SampleShadowFeature::subscribeToPertinentShadowTopics()
{
    UpdateNamedShadowSubscriptionRequest updateNamedShadowSubscriptionRequest;
    updateNamedShadowSubscriptionRequest.ThingName = thingName.c_str();
    updateNamedShadowSubscriptionRequest.ShadowName = shadowName.c_str();

    shadowClient->SubscribeToUpdateNamedShadowAccepted(
        updateNamedShadowSubscriptionRequest,
        AWS_MQTT_QOS_AT_LEAST_ONCE,
        std::bind(
            &SampleShadowFeature::updateNamedShadowAcceptedHandler, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&SampleShadowFeature::ackSubscribeToUpdateNamedShadowAccepted, this, std::placeholders::_1));

    shadowClient->SubscribeToUpdateNamedShadowRejected(
        updateNamedShadowSubscriptionRequest,
        AWS_MQTT_QOS_AT_LEAST_ONCE,
        std::bind(
            &SampleShadowFeature::updateNamedShadowRejectedHandler, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&SampleShadowFeature::ackSubscribeToUpdateNamedShadowRejected, this, std::placeholders::_1));

    NamedShadowUpdatedSubscriptionRequest namedShadowUpdatedSubscriptionRequest;
    namedShadowUpdatedSubscriptionRequest.ThingName = thingName.c_str();
    namedShadowUpdatedSubscriptionRequest.ShadowName = shadowName.c_str();

    shadowClient->SubscribeToNamedShadowUpdatedEvents(
        namedShadowUpdatedSubscriptionRequest,
        AWS_MQTT_QOS_AT_LEAST_ONCE,
        std::bind(
            &SampleShadowFeature::updateNamedShadowEventHandler, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&SampleShadowFeature::ackSubscribeToUpdateEvent, this, std::placeholders::_1));

    NamedShadowDeltaUpdatedSubscriptionRequest namedShadowDeltaUpdatedSubscriptionRequest;
    namedShadowDeltaUpdatedSubscriptionRequest.ThingName = thingName.c_str();
    namedShadowDeltaUpdatedSubscriptionRequest.ShadowName = shadowName.c_str();

    shadowClient->SubscribeToNamedShadowDeltaUpdatedEvents(
        namedShadowDeltaUpdatedSubscriptionRequest,
        AWS_MQTT_QOS_AT_LEAST_ONCE,
        std::bind(
            &SampleShadowFeature::updateNamedShadowDeltaHandler, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&SampleShadowFeature::ackSubscribeToUpdateDelta, this, std::placeholders::_1));

    // check if we have subscribed all update topic successfully before making the call
    auto futureSubscribeShadowUpdateDeltaPromise = subscribeShadowUpdateDeltaPromise.get_future();
    auto futureSubscribeShadowUpdateEventPromise = subscribeShadowUpdateEventPromise.get_future();
    auto futureSubscribeShadowUpdateRejectedPromise = subscribeShadowUpdateRejectedPromise.get_future();
    auto futureSubscribeShadowUpdateAcceptedPromise = subscribeShadowUpdateAcceptedPromise.get_future();

    if (futureSubscribeShadowUpdateAcceptedPromise.wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
            future_status::timeout ||
        futureSubscribeShadowUpdateRejectedPromise.wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
            future_status::timeout ||
        futureSubscribeShadowUpdateDeltaPromise.wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
            future_status::timeout ||
        futureSubscribeShadowUpdateEventPromise.wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
            future_status::timeout)
    {
        LOGM_ERROR(TAG, "Subscribing to pertinent %s shadowUpdate topics timed out", shadowName.c_str());
        return false;
    }

    if (!futureSubscribeShadowUpdateAcceptedPromise.get() || !futureSubscribeShadowUpdateRejectedPromise.get() ||
        !futureSubscribeShadowUpdateDeltaPromise.get() || !futureSubscribeShadowUpdateEventPromise.get())
    {
        return false;
    }

    return true;
}

void SampleShadowFeature::readAndUpdateShadowFromFile()
{

    Crt::JsonObject jsonObj;

    if (inputFile.empty())
    {
        jsonObj.WithString("welcome", "aws-iot");
    }
    else
    {
        string expandedPath = inputFile;

        ifstream setting(expandedPath.c_str());
        if (!setting.is_open())
        {
            LOGM_ERROR(TAG, "Unable to open file: '%s'", Sanitize(expandedPath).c_str());
            return;
        }

        std::string contents((std::istreambuf_iterator<char>(setting)), std::istreambuf_iterator<char>());
        jsonObj = Aws::Crt::JsonObject(contents.c_str());
        if (!jsonObj.WasParseSuccessful())
        {
            LOGM_ERROR(
                TAG,
                "Couldn't parse JSON shadow data file. GetErrorMessage returns: %s",
                jsonObj.GetErrorMessage().c_str());
            return;
        }
        setting.close();
    }

    UpdateNamedShadowRequest updateNamedShadowRequest;
    updateNamedShadowRequest.ThingName = thingName.c_str();
    updateNamedShadowRequest.ShadowName = shadowName.c_str();

    ShadowState state;
    state.Reported = jsonObj;
    updateNamedShadowRequest.State = state;

    Aws::Crt::UUID uuid;
    updateNamedShadowRequest.ClientToken = uuid.ToString();

    shadowClient->PublishUpdateNamedShadow(
        updateNamedShadowRequest,
        AWS_MQTT_QOS_AT_LEAST_ONCE,
        std::bind(&SampleShadowFeature::ackUpdateNamedShadowStatus, this, std::placeholders::_1));
}

int SampleShadowFeature::start()
{
    LOGM_INFO(TAG, "Starting %s", getName().c_str());

    shadowClient = unique_ptr<IotShadowClient>(new IotShadowClient(resourceManager.get()->getConnection()));

    if (!subscribeToPertinentShadowTopics())
    {
        LOGM_ERROR(TAG, "Failed to subscribe to related %s shadow topics", shadowName.c_str());
        this->stop();
    }

    readAndUpdateShadowFromFile();

    if (!inputFile.empty())
    {
        thread file_monitor_thread(&SampleShadowFeature::runFileMonitor, this);
        file_monitor_thread.detach();
    }

    baseNotifier->onEvent(static_cast<Feature *>(this), ClientBaseEventNotification::FEATURE_STARTED);
    return AWS_OP_SUCCESS;
}

int SampleShadowFeature::stop()
{
    needStop.store(true);
    baseNotifier->onEvent(static_cast<Feature *>(this), ClientBaseEventNotification::FEATURE_STOPPED);
    return AWS_OP_SUCCESS;
}