// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "ConfigShadow.h"
#include "../config/Config.h"
#include "../logging/LoggerFactory.h"
#include <aws/crt/UUID.h>
#include <aws/iotshadow/ErrorResponse.h>
#include <aws/iotshadow/GetNamedShadowRequest.h>
#include <aws/iotshadow/GetNamedShadowSubscriptionRequest.h>
#include <aws/iotshadow/GetShadowResponse.h>
#include <aws/iotshadow/UpdateNamedShadowRequest.h>
#include <aws/iotshadow/UpdateNamedShadowSubscriptionRequest.h>

using namespace std;
using namespace Aws;
using namespace Aws::Iot;
using namespace Aws::Crt;
using namespace Aws::Iotshadow;
using namespace Aws::Iot::DeviceClient::Shadow;
using namespace Aws::Iot::DeviceClient::Util;
using namespace Aws::Iot::DeviceClient::Logging;

constexpr char ConfigShadow::TAG[];
constexpr char ConfigShadow::DEFAULT_CONFIG_SHADOW_NAME[];
constexpr int ConfigShadow::DEFAULT_WAIT_TIME_SECONDS;
void ConfigShadow::getNamedShadowRejectedHandler(Iotshadow::ErrorResponse *errorResponse, int ioError)
{
    if (ioError)
    {
        LOGM_ERROR(TAG, "Encountered ioError %d within getNamedShadowRejectedHandler", ioError);
    }

    if (errorResponse->Message.has_value())
    {
        LOGM_ERROR(TAG, "getNamedShadowRequest gets rejected: %s", errorResponse->Message->c_str());
    }
    configShadowExistsPromise.set_value(ioError == AWS_OP_SUCCESS);
}

void ConfigShadow::getNamedShadowAcceptedHandler(Iotshadow::GetShadowResponse *response, int ioError)
{
    if (ioError)
    {
        LOGM_ERROR(TAG, "Encountered ioError %d within getNamedShadowAcceptedHandler", ioError);
    }

    if (response->State->Delta.has_value())
    {
        configDelta = response->State->Delta.value();
    }

    desiredConfig = response->State->Desired.value();
    configShadowExistsPromise.set_value(ioError == AWS_OP_SUCCESS);
}

void ConfigShadow::updateNamedShadowAcceptedHandler(Iotshadow::UpdateShadowResponse *response, int ioError) const
{
    if (ioError)
    {
        LOGM_ERROR(TAG, "Encountered ioError %d within updateNamedShadowAcceptedHandler", ioError);
    }
}

void ConfigShadow::updateNamedShadowRejectedHandler(Iotshadow::ErrorResponse *errorResponse, int ioError) const
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

void ConfigShadow::ackSubscribeToUpdateNamedShadowAccepted(int ioError)
{
    LOGM_DEBUG(TAG, "Ack received for SubscribeToUpdateNamedShadowAccepted with code {%d}", ioError);
    if (ioError)
    {
        string errorMessage = "Encountered an ioError while attempting to subscribe to UpdateNamedShadowAccepted";
        LOG_ERROR(TAG, errorMessage.c_str());
    }
    subscribeShadowUpdateAcceptedPromise.set_value(ioError == AWS_OP_SUCCESS);
}

void ConfigShadow::ackSubscribeToUpdateNamedShadowRejected(int ioError)
{
    LOGM_DEBUG(TAG, "Ack received for SubscribeToUpdateNamedShadowRejected with code {%d}", ioError);
    if (ioError)
    {
        string errorMessage = "Encountered an ioError while attempting to subscribe to UpdateNamedShadowRejected";
        LOG_ERROR(TAG, errorMessage.c_str());
    }
    subscribeShadowUpdateRejectedPromise.set_value(ioError == AWS_OP_SUCCESS);
}

void ConfigShadow::ackSubscribeToGetNamedShadowAccepted(int ioError)
{
    LOGM_DEBUG(TAG, "Ack received for SubscribeToGetNamedShadowAccepted with code {%d}", ioError);
    if (ioError)
    {
        string errorMessage = "Encountered an ioError while attempting to subscribe to GetNamedShadowAccepted";
        LOG_ERROR(TAG, errorMessage.c_str());
    }
    subscribeShadowGetAcceptedPromise.set_value(ioError == AWS_OP_SUCCESS);
}

void ConfigShadow::ackSubscribeToGetNamedShadowRejected(int ioError)
{
    LOGM_DEBUG(TAG, "Ack received for SubscribeToGetNamedShadowRejected with code {%d}", ioError);
    if (ioError)
    {
        string errorMessage = "Encountered an ioError while attempting to subscribe to GetNamedShadowRejected";
        LOG_ERROR(TAG, errorMessage.c_str());
    }
    subscribeShadowGetRejectedPromise.set_value(ioError == AWS_OP_SUCCESS);
}

void ConfigShadow::ackGetNamedShadowStatus(int ioError)
{
    LOGM_DEBUG(TAG, "Ack received for getNamedShadowStatus with code {%d}", ioError);
    shadowGetCompletedPromise.set_value(ioError == AWS_OP_SUCCESS);
}

void ConfigShadow::ackUpdateNamedShadowStatus(int ioError)
{
    LOGM_DEBUG(TAG, "Ack received for updateNamedShadowStatus with code {%d}", ioError);
    shadowUpdateCompletedPromise.set_value(ioError == AWS_OP_SUCCESS);
}

bool ConfigShadow::subscribeGetAndUpdateNamedShadowTopics(Iotshadow::IotShadowClient iotShadowClient)
{
    GetNamedShadowSubscriptionRequest getNamedShadowSubscriptionRequest;
    getNamedShadowSubscriptionRequest.ShadowName = DEFAULT_CONFIG_SHADOW_NAME;
    getNamedShadowSubscriptionRequest.ThingName = thingName.c_str();

    iotShadowClient.SubscribeToGetNamedShadowAccepted(
        getNamedShadowSubscriptionRequest,
        AWS_MQTT_QOS_AT_LEAST_ONCE,
        std::bind(&ConfigShadow::getNamedShadowAcceptedHandler, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&ConfigShadow::ackSubscribeToGetNamedShadowAccepted, this, std::placeholders::_1));

    iotShadowClient.SubscribeToGetNamedShadowRejected(
        getNamedShadowSubscriptionRequest,
        AWS_MQTT_QOS_AT_LEAST_ONCE,
        std::bind(&ConfigShadow::getNamedShadowRejectedHandler, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&ConfigShadow::ackSubscribeToGetNamedShadowRejected, this, std::placeholders::_1));

    auto futureSubscribeShadowGetAcceptedPromise = subscribeShadowGetAcceptedPromise.get_future();
    auto futureSubscribeShadowGetRejectedPromise = subscribeShadowGetRejectedPromise.get_future();

    if (futureSubscribeShadowGetAcceptedPromise.wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
            future_status::timeout ||
        futureSubscribeShadowGetRejectedPromise.wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
            future_status::timeout)
    {
        LOGM_ERROR(TAG, "Subscribing to pertinent %s shadowGet topics timed out.", DEFAULT_CONFIG_SHADOW_NAME);
        return false;
    }

    if (!futureSubscribeShadowGetAcceptedPromise.get() || !futureSubscribeShadowGetRejectedPromise.get())
    {
        return false;
    }

    UpdateNamedShadowSubscriptionRequest updateNamedShadowSubscriptionRequest;
    updateNamedShadowSubscriptionRequest.ThingName = thingName.c_str();
    updateNamedShadowSubscriptionRequest.ShadowName = DEFAULT_CONFIG_SHADOW_NAME;

    iotShadowClient.SubscribeToUpdateNamedShadowAccepted(
        updateNamedShadowSubscriptionRequest,
        AWS_MQTT_QOS_AT_LEAST_ONCE,
        std::bind(&ConfigShadow::updateNamedShadowAcceptedHandler, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&ConfigShadow::ackSubscribeToUpdateNamedShadowAccepted, this, std::placeholders::_1));

    iotShadowClient.SubscribeToUpdateNamedShadowRejected(
        updateNamedShadowSubscriptionRequest,
        AWS_MQTT_QOS_AT_LEAST_ONCE,
        std::bind(&ConfigShadow::updateNamedShadowRejectedHandler, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&ConfigShadow::ackSubscribeToUpdateNamedShadowRejected, this, std::placeholders::_1));

    auto futureSubscribeShadowUpdateRejectedPromise = subscribeShadowUpdateRejectedPromise.get_future();
    auto futureSubscribeShadowUpdateAcceptedPromise = subscribeShadowUpdateAcceptedPromise.get_future();

    if (futureSubscribeShadowUpdateAcceptedPromise.wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
            future_status::timeout ||
        futureSubscribeShadowUpdateRejectedPromise.wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
            future_status::timeout)
    {
        LOGM_ERROR(TAG, "Subscribing to pertinent %s shadowUpdate topics timed out.", DEFAULT_CONFIG_SHADOW_NAME);
        return false;
    }

    if (!futureSubscribeShadowUpdateAcceptedPromise.get() || !futureSubscribeShadowUpdateRejectedPromise.get())
    {
        return false;
    }

    return true;
}

bool ConfigShadow::fetchRemoteConfigShadow(Iotshadow::IotShadowClient iotShadowClient)
{
    GetNamedShadowRequest getNamedShadowRequest;
    Aws::Crt::UUID uuid;
    getNamedShadowRequest.ShadowName = DEFAULT_CONFIG_SHADOW_NAME;
    getNamedShadowRequest.ThingName = thingName.c_str();
    getNamedShadowRequest.ClientToken = uuid.ToString();

    iotShadowClient.PublishGetNamedShadow(
        getNamedShadowRequest,
        AWS_MQTT_QOS_AT_LEAST_ONCE,
        std::bind(&ConfigShadow::ackGetNamedShadowStatus, this, std::placeholders::_1));

    auto futureShadowGetCompletedPromise = shadowGetCompletedPromise.get_future();
    if (futureShadowGetCompletedPromise.wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
        future_status::timeout)
    {
        LOGM_ERROR(TAG, "Publishing to %s shadowGet topic timed out.", DEFAULT_CONFIG_SHADOW_NAME);
        return false;
    }

    return futureShadowGetCompletedPromise.get();
}

void ConfigShadow::loadFeatureConfigIntoJsonObject(PlainConfig &config, Aws::Crt::JsonObject &jsonObj) const
{
    JsonObject tunneling;
    config.tunneling.SerializeToObject(tunneling);
    jsonObj.WithObject(PlainConfig::JSON_KEY_TUNNELING, tunneling);

    JsonObject jobs;
    config.jobs.SerializeToObject(jobs);
    jsonObj.WithObject(PlainConfig::JSON_KEY_JOBS, jobs);

    JsonObject deviceDefender;
    config.deviceDefender.SerializeToObject(deviceDefender);
    jsonObj.WithObject(PlainConfig::JSON_KEY_DEVICE_DEFENDER, deviceDefender);

    JsonObject samples;
    JsonObject pubsub;
    config.pubSub.SerializeToObject(pubsub);
    samples.WithObject(PlainConfig::JSON_KEY_PUB_SUB, pubsub);
    jsonObj.WithObject(PlainConfig::JSON_KEY_SAMPLES, samples);

    JsonObject sampleShadow;
    config.sampleShadow.SerializeToObject(sampleShadow);

    jsonObj.WithObject(PlainConfig::JSON_KEY_SAMPLE_SHADOW, sampleShadow);
}

void ConfigShadow::resetClientConfigWithJSON(
    PlainConfig &config,
    Crt::JsonView &deltaJsonView,
    Crt::JsonView &desiredJsonView) const
{
    if (desiredJsonView.ValueExists(PlainConfig::JSON_KEY_JOBS) &&
        deltaJsonView.ValueExists(PlainConfig::JSON_KEY_JOBS))
    {
        PlainConfig::Jobs jobs;
        jobs.LoadFromJson(desiredJsonView.GetJsonObject(PlainConfig::JSON_KEY_JOBS));
        if (jobs.Validate())
        {
            config.jobs = jobs;
        }
        else
        {
            LOGM_WARN(
                TAG,
                "Config shadow contains invalid configurations in %s feature, aborting this feature's configuration "
                "update now. Please check the error logs for more information",
                PlainConfig::JSON_KEY_JOBS);
        }
    }

    if (desiredJsonView.ValueExists(PlainConfig::JSON_KEY_TUNNELING) &&
        deltaJsonView.ValueExists(PlainConfig::JSON_KEY_TUNNELING))
    {
        PlainConfig::Tunneling tunneling;
        tunneling.LoadFromJson(desiredJsonView.GetJsonObject(PlainConfig::JSON_KEY_TUNNELING));
        if (tunneling.Validate())
        {
            config.tunneling = tunneling;
        }
        else
        {
            LOGM_WARN(
                TAG,
                "Config shadow contains invalid configurations in %s feature, aborting this feature's configuration "
                "update now. Please check the error logs for more information",
                PlainConfig::JSON_KEY_TUNNELING);
        }
    }

    if (desiredJsonView.ValueExists(PlainConfig::JSON_KEY_DEVICE_DEFENDER) &&
        deltaJsonView.ValueExists(PlainConfig::JSON_KEY_DEVICE_DEFENDER))
    {
        PlainConfig::DeviceDefender deviceDefender;
        deviceDefender.LoadFromJson(desiredJsonView.GetJsonObject(PlainConfig::JSON_KEY_DEVICE_DEFENDER));
        if (deviceDefender.Validate())
        {
            config.deviceDefender = deviceDefender;
        }
        else
        {
            LOGM_WARN(
                TAG,
                "Config shadow contains invalid configurations in %s feature, aborting this feature's configuration "
                "update now. Please check the error logs for more information",
                PlainConfig::JSON_KEY_DEVICE_DEFENDER);
        }
    }

    if (desiredJsonView.ValueExists(PlainConfig::JSON_KEY_SAMPLES) &&
        deltaJsonView.ValueExists(PlainConfig::JSON_KEY_SAMPLES))
    {
        PlainConfig::PubSub pubSub;
        pubSub.LoadFromJson(
            desiredJsonView.GetJsonObject(PlainConfig::JSON_KEY_SAMPLES).GetJsonObject(PlainConfig::JSON_KEY_PUB_SUB));
        if (pubSub.Validate())
        {
            config.pubSub = pubSub;
        }
        else
        {
            LOGM_WARN(
                TAG,
                "Config shadow contains invalid configurations in %s feature, aborting this feature's configuration "
                "update now. Please check the error logs for more information",
                PlainConfig::JSON_KEY_PUB_SUB);
        }
    }

    if (desiredJsonView.ValueExists(PlainConfig::JSON_KEY_SAMPLE_SHADOW) &&
        deltaJsonView.ValueExists(PlainConfig::JSON_KEY_SAMPLE_SHADOW))
    {
        PlainConfig::SampleShadow sampleShadow;
        sampleShadow.LoadFromJson(desiredJsonView.GetJsonObject(PlainConfig::JSON_KEY_SAMPLE_SHADOW));
        if (sampleShadow.Validate())
        {
            config.sampleShadow = sampleShadow;
        }
        else
        {
            LOGM_WARN(
                TAG,
                "Config shadow contains invalid configurations in %s feature, aborting this feature's configuration "
                "update now. Please check the error logs for more information",
                PlainConfig::JSON_KEY_SAMPLE_SHADOW);
        }
    }
}

void ConfigShadow::updateShadowWithLocalConfig(Iotshadow::IotShadowClient iotShadowClient, PlainConfig &config)
{
    JsonObject jsonObj;
    loadFeatureConfigIntoJsonObject(config, jsonObj);

    UpdateNamedShadowRequest updateNamedShadowRequest;
    updateNamedShadowRequest.ThingName = thingName.c_str();
    updateNamedShadowRequest.ShadowName = DEFAULT_CONFIG_SHADOW_NAME;

    ShadowState state;
    state.Reported = jsonObj;
    state.Desired = jsonObj;
    updateNamedShadowRequest.State = state;

    Aws::Crt::UUID uuid;
    updateNamedShadowRequest.ClientToken = uuid.ToString();

    iotShadowClient.PublishUpdateNamedShadow(
        updateNamedShadowRequest,
        AWS_MQTT_QOS_AT_LEAST_ONCE,
        std::bind(&ConfigShadow::ackUpdateNamedShadowStatus, this, std::placeholders::_1));
    auto futureShadowUpdateCompletedPromise = shadowUpdateCompletedPromise.get_future();

    if (futureShadowUpdateCompletedPromise.wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
        future_status::timeout)
    {
        LOGM_ERROR(TAG, "Publishing to shadowUpdate topic timed out.", DEFAULT_CONFIG_SHADOW_NAME);
        return;
    }

    if (!futureShadowUpdateCompletedPromise.get())
    {
        LOGM_ERROR(
            TAG,
            "Encountering unexpected error while publishing the request to %s updateNamedShadow topic",
            DEFAULT_CONFIG_SHADOW_NAME);
    }
}

void ConfigShadow::reconfigureWithConfigShadow(
    std::shared_ptr<SharedCrtResourceManager> resourceManager,
    PlainConfig &config)
{
    IotShadowClient iotShadowClient(resourceManager.get()->getConnection());
    thingName = *config.thingName;
    if (!subscribeGetAndUpdateNamedShadowTopics(iotShadowClient))
    {
        LOGM_ERROR(
            TAG, "Encounter error while subscribing pertinent %s shadow topic from cloud", DEFAULT_CONFIG_SHADOW_NAME);
        return;
    }

    if (!fetchRemoteConfigShadow(iotShadowClient))
    {
        LOGM_ERROR(
            TAG, "Encounter error while fetching device client config shadow from cloud", DEFAULT_CONFIG_SHADOW_NAME);
        return;
    }

    auto futureConfigShadowExistsPromise = configShadowExistsPromise.get_future();
    if (futureConfigShadowExistsPromise.wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
        future_status::timeout)
    {
        LOGM_ERROR(
            TAG,
            "Waiting for %s getNamedShadow response time out or get unexpected error from service",
            DEFAULT_CONFIG_SHADOW_NAME);
        return;
    }

    if (futureConfigShadowExistsPromise.get() && configDelta.has_value())
    {
        // config shadow and delta exists, resetting the local config first and then updating config shaodw
        if (configDelta)
        {
            LOG_INFO(
                TAG, "Detect the delta of configuration in the config shadow, reconfiguring the device client now.");

            if (!desiredConfig.has_value())
            {
                LOG_ERROR(
                    TAG, "Fail to fetch the desired value in config shaodw, aborting the configuration update now");
                return;
            }

            JsonObject configDesiredObject = configDelta.value();
            if (!configDesiredObject.WasParseSuccessful())
            {
                LOGM_ERROR(
                    TAG,
                    "Couldn't parse JSON config desired. GetErrorMessage returns: %s",
                    configDesiredObject.GetErrorMessage().c_str());
                return;
            }

            JsonObject configDeltaObject = configDelta.value();
            if (!configDeltaObject.WasParseSuccessful())
            {
                LOGM_ERROR(
                    TAG,
                    "Couldn't parse JSON config delta. GetErrorMessage returns: %s",
                    configDeltaObject.GetErrorMessage().c_str());
                return;
            }

            JsonView desiredJsonView = desiredConfig->View();
            JsonView deltaJsonView = configDelta->View();
            resetClientConfigWithJSON(config, deltaJsonView, desiredJsonView);
        }
        updateShadowWithLocalConfig(iotShadowClient, config);
    }
    else
    {
        // config shadow doesn't exists, store the device client configuration into config shadow
        updateShadowWithLocalConfig(iotShadowClient, config);
    }
}
