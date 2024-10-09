// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "Config.h"

#if !defined(EXCLUDE_JOBS)

#    include "../jobs/JobsFeature.h"

#endif

#include "../logging/LoggerFactory.h"

#if !defined(EXCLUDE_ST)

#    include "../tunneling/SecureTunnelingFeature.h"

#endif

#include "../SharedCrtResourceManager.h"
#include "../util/FileUtils.h"
#include "../util/MqttUtils.h"
#include "../util/ProxyUtils.h"
#include "../util/StringUtils.h"
#include "Version.h"

#include <algorithm>
#include <aws/crt/JsonObject.h>
#include <aws/io/socket.h>
#include <cstdlib>
#include <iostream>
#include <map>
#include <regex>
#include <stdexcept>
#include <string>
#include <sys/stat.h>

using namespace std;
using namespace Aws::Iot;
using namespace Aws::Iot::DeviceClient;
#if !defined(EXCLUDE_ST)
using namespace Aws::Iot::DeviceClient::SecureTunneling;
#endif
using namespace Aws::Iot::DeviceClient::Logging;
using namespace Aws::Iot::DeviceClient::Util;

constexpr char PlainConfig::CLI_ENDPOINT[];
constexpr char PlainConfig::CLI_CERT[];
constexpr char PlainConfig::CLI_KEY[];
constexpr char PlainConfig::CLI_ROOT_CA[];
constexpr char PlainConfig::CLI_THING_NAME[];
constexpr char PlainConfig::JSON_KEY_ENDPOINT[];
constexpr char PlainConfig::JSON_KEY_CERT[];
constexpr char PlainConfig::JSON_KEY_KEY[];
constexpr char PlainConfig::JSON_KEY_ROOT_CA[];
constexpr char PlainConfig::JSON_KEY_THING_NAME[];
constexpr char PlainConfig::JSON_KEY_LOGGING[];
constexpr char PlainConfig::JSON_KEY_JOBS[];
constexpr char PlainConfig::JSON_KEY_TUNNELING[];
constexpr char PlainConfig::JSON_KEY_DEVICE_DEFENDER[];
constexpr char PlainConfig::JSON_KEY_FLEET_PROVISIONING[];
constexpr char PlainConfig::JSON_KEY_RUNTIME_CONFIG[];
constexpr char PlainConfig::JSON_KEY_SAMPLES[];
constexpr char PlainConfig::JSON_KEY_PUB_SUB[];
constexpr char PlainConfig::JSON_KEY_SAMPLE_SHADOW[];
constexpr char PlainConfig::JSON_KEY_CONFIG_SHADOW[];
constexpr char PlainConfig::JSON_KEY_SECURE_ELEMENT[];
constexpr char PlainConfig::JSON_KEY_SENSOR_PUBLISH[];
constexpr char PlainConfig::DEFAULT_LOCK_FILE_PATH[];

constexpr int Permissions::KEY_DIR;
constexpr int Permissions::ROOT_CA_DIR;
constexpr int Permissions::CERT_DIR;
constexpr int Permissions::CONFIG_DIR;
constexpr int Permissions::LOG_DIR;
constexpr int Permissions::PKCS11_LIB_DIR;
constexpr int Permissions::PRIVATE_KEY;
constexpr int Permissions::PUBLIC_CERT;
constexpr int Permissions::LOG_FILE;
constexpr int Permissions::CONFIG_FILE;
constexpr int Permissions::RUNTIME_CONFIG_FILE;
constexpr int Permissions::JOB_HANDLER;
constexpr int Permissions::PUB_SUB_FILES;
constexpr int Permissions::SAMPLE_SHADOW_FILES;
constexpr int Permissions::PKCS11_LIB_FILE;

bool PlainConfig::LoadFromJson(const Crt::JsonView &json)
{
    const char *jsonKey = JSON_KEY_ENDPOINT;
    if (json.ValueExists(jsonKey))
    {
        endpoint = json.GetString(jsonKey).c_str();
    }

    jsonKey = JSON_KEY_CERT;
    if (json.ValueExists(jsonKey))
    {
        if (!json.GetString(jsonKey).empty())
        {
            cert = FileUtils::ExtractExpandedPath(json.GetString(jsonKey).c_str());
        }
        else
        {
            LOGM_WARN(Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
        }
    }

    jsonKey = JSON_KEY_KEY;
    if (json.ValueExists(jsonKey))
    {
        if (!json.GetString(jsonKey).empty())
        {
            key = FileUtils::ExtractExpandedPath(json.GetString(jsonKey).c_str());
        }
        else
        {
            LOGM_WARN(Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
        }
    }

    jsonKey = JSON_KEY_ROOT_CA;
    if (json.ValueExists(jsonKey))
    {
        if (!json.GetString(jsonKey).empty())
        {
            auto path = FileUtils::ExtractExpandedPath(json.GetString(jsonKey).c_str());
            if (FileUtils::FileExists(path))
            {
                rootCa = path;
            }
            else
            {
                LOGM_WARN(
                    Config::TAG,
                    "Path %s to RootCA is invalid. Ignoring... Will attempt to use default trust store.",
                    path.c_str());
            }
        }
        else
        {
            LOGM_WARN(Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
        }
    }

    jsonKey = JSON_KEY_THING_NAME;
    if (json.ValueExists(jsonKey))
    {
        thingName = json.GetString(jsonKey).c_str();
    }

    jsonKey = JSON_KEY_JOBS;
    if (json.ValueExists(jsonKey))
    {
        Jobs temp;
        temp.LoadFromJson(json.GetJsonObject(jsonKey));
        jobs = temp;
    }

    jsonKey = JSON_KEY_TUNNELING;
    if (json.ValueExists(jsonKey))
    {
        Tunneling temp;
        temp.LoadFromJson(json.GetJsonObject(jsonKey));
        tunneling = temp;
    }

    jsonKey = JSON_KEY_DEVICE_DEFENDER;
    if (json.ValueExists(jsonKey))
    {
        DeviceDefender temp;
        temp.LoadFromJson(json.GetJsonObject(jsonKey));
        deviceDefender = temp;
    }

    jsonKey = JSON_KEY_FLEET_PROVISIONING;
    if (json.ValueExists(jsonKey))
    {
        FleetProvisioning temp;
        temp.LoadFromJson(json.GetJsonObject(jsonKey));
        fleetProvisioning = temp;
    }

    jsonKey = JSON_KEY_RUNTIME_CONFIG;
    if (json.ValueExists(jsonKey))
    {
        FleetProvisioningRuntimeConfig temp;
        temp.LoadFromJson(json.GetJsonObject(jsonKey));
        fleetProvisioningRuntimeConfig = temp;
    }

    jsonKey = JSON_KEY_LOGGING;
    if (json.ValueExists(jsonKey))
    {
        LogConfig temp;
        temp.LoadFromJson(json.GetJsonObject(jsonKey));
        logConfig = temp;
    }

    jsonKey = JSON_KEY_SAMPLES;
    if (json.ValueExists(jsonKey))
    {
        const char *jsonKeyTwo = JSON_KEY_PUB_SUB;
        if (json.GetJsonObject(jsonKey).ValueExists(jsonKeyTwo))
        {
            PubSub temp;
            temp.LoadFromJson(json.GetJsonObject(jsonKey).GetJsonObject(jsonKeyTwo));
            pubSub = temp;
        }
    }

    jsonKey = JSON_KEY_SAMPLE_SHADOW;
    if (json.ValueExists(jsonKey))
    {
        SampleShadow temp;
        temp.LoadFromJson(json.GetJsonObject(jsonKey));
        sampleShadow = temp;
    }

    jsonKey = JSON_KEY_CONFIG_SHADOW;
    if (json.ValueExists(jsonKey))
    {
        ConfigShadow temp;
        temp.LoadFromJson(json.GetJsonObject(jsonKey));
        configShadow = temp;
    }

    jsonKey = JSON_KEY_SECURE_ELEMENT;
    if (json.ValueExists(jsonKey))
    {
        SecureElement temp;
        temp.LoadFromJson(json.GetJsonObject(jsonKey));
        secureElement = temp;
    }

    jsonKey = JSON_KEY_SENSOR_PUBLISH;
    if (json.ValueExists(jsonKey))
    {
        SensorPublish temp;
        temp.LoadFromJson(json.GetJsonObject(jsonKey));
        sensorPublish = temp;
    }

    return true;
}

bool PlainConfig::LoadFromCliArgs(const CliArgs &cliArgs)
{
    if (cliArgs.count(PlainConfig::CLI_ENDPOINT))
    {
        endpoint = cliArgs.at(PlainConfig::CLI_ENDPOINT).c_str();
    }
    if (cliArgs.count(PlainConfig::CLI_CERT))
    {
        cert = FileUtils::ExtractExpandedPath(cliArgs.at(PlainConfig::CLI_CERT).c_str());
    }
    if (cliArgs.count(PlainConfig::CLI_KEY))
    {
        key = FileUtils::ExtractExpandedPath(cliArgs.at(PlainConfig::CLI_KEY).c_str());
    }
    if (cliArgs.count(PlainConfig::CLI_ROOT_CA))
    {
        auto path = FileUtils::ExtractExpandedPath(cliArgs.at(PlainConfig::CLI_ROOT_CA).c_str());
        if (FileUtils::IsValidFilePath(path))
        {
            rootCa = FileUtils::ExtractExpandedPath(cliArgs.at(PlainConfig::CLI_ROOT_CA).c_str());
        }
        else
        {
            LOGM_WARN(
                Config::TAG,
                "Path %s to RootCA is invalid. Ignoring... Will attempt to use default trust store.",
                path.c_str());
        }
    }
    if (cliArgs.count(PlainConfig::CLI_THING_NAME))
    {
        thingName = cliArgs.at(PlainConfig::CLI_THING_NAME).c_str();
    }

    bool loadFeatureCliArgs = tunneling.LoadFromCliArgs(cliArgs) && logConfig.LoadFromCliArgs(cliArgs) &&
                              httpProxyConfig.LoadFromCliArgs(cliArgs);
#if !defined(DISABLE_MQTT)
    loadFeatureCliArgs = loadFeatureCliArgs && jobs.LoadFromCliArgs(cliArgs) &&
                         deviceDefender.LoadFromCliArgs(cliArgs) && fleetProvisioning.LoadFromCliArgs(cliArgs) &&
                         pubSub.LoadFromCliArgs(cliArgs) && sampleShadow.LoadFromCliArgs(cliArgs) &&
                         configShadow.LoadFromCliArgs(cliArgs) && secureElement.LoadFromCliArgs(cliArgs);
#endif
    return loadFeatureCliArgs;
}

bool PlainConfig::LoadFromEnvironment()
{
    const char *lockFilePathIn = std::getenv("LOCK_FILE_PATH");
    if (lockFilePathIn)
    {
        string lockFilePathStr = FileUtils::ExtractExpandedPath(lockFilePathIn);
        if (!lockFilePathStr.empty() && lockFilePathStr.back() != '/')
        {
            lockFilePathStr += '/';
        }

        LOGM_DEBUG(Config::TAG, "Set LOCK_FILE_PATH=%s", Sanitize(lockFilePathStr).c_str());
        lockFilePath = lockFilePathStr;
    }

    bool loadFeatureEnvironmentVar = tunneling.LoadFromEnvironment() && logConfig.LoadFromEnvironment();
#if !defined(DISABLE_MQTT)
    loadFeatureEnvironmentVar = loadFeatureEnvironmentVar && jobs.LoadFromEnvironment() &&
                                deviceDefender.LoadFromEnvironment() && fleetProvisioning.LoadFromEnvironment() &&
                                fleetProvisioningRuntimeConfig.LoadFromEnvironment() && pubSub.LoadFromEnvironment() &&
                                sampleShadow.LoadFromEnvironment() && configShadow.LoadFromEnvironment();
#endif
    return loadFeatureEnvironmentVar;
}

bool PlainConfig::Validate() const
{
    if (!logConfig.Validate())
    {
        return false;
    }
    if (rootCa.has_value() && !rootCa->empty() && FileUtils::FileExists(rootCa->c_str()))
    {
        string parentDir = FileUtils::ExtractParentDirectory(rootCa->c_str());
        if (!FileUtils::ValidateFilePermissions(parentDir, Permissions::ROOT_CA_DIR) ||
            !FileUtils::ValidateFilePermissions(rootCa->c_str(), Permissions::ROOT_CA))
        {
            LOG_ERROR(Config::TAG, "Incorrect permissions on Root CA file and/or parent directory");
            return false;
        }
    }
#if !defined(DISABLE_MQTT)
    if (!endpoint.has_value() || endpoint->empty())
    {
        LOGM_ERROR(Config::TAG, "*** %s: Endpoint is missing ***", DeviceClient::DC_FATAL_ERROR);
        return false;
    }
    if (!cert.has_value() || cert->empty())
    {
        LOGM_ERROR(Config::TAG, "*** %s: Certificate is missing ***", DeviceClient::DC_FATAL_ERROR);
        return false;
    }
    else if (!FileUtils::IsValidFilePath(cert->c_str()))
    {
        return false;
    }

    if (!secureElement.enabled)
    {
        if (!key.has_value() || key->empty())
        {
            LOGM_ERROR(Config::TAG, "*** %s: Private Key is missing ***", DeviceClient::DC_FATAL_ERROR);
            return false;
        }
        else if (!FileUtils::IsValidFilePath(key->c_str()))
        {
            return false;
        }
    }

    if (!thingName.has_value() || thingName->empty())
    {
        LOGM_ERROR(Config::TAG, "*** %s: Thing name is missing ***", DeviceClient::DC_FATAL_ERROR);
        return false;
    }
#endif
#if !defined(EXCLUDE_JOBS) || !defined(DISABLE_MQTT)
    if (!jobs.Validate())
    {
        return false;
    }
#endif
#if !defined(EXCLUDE_DD) || !defined(DISABLE_MQTT)
    if (!deviceDefender.Validate())
    {
        return false;
    }
#endif
#if !defined(EXCLUDE_ST)
    if (!tunneling.Validate())
    {
        return false;
    }
#endif
#if !defined(EXCLUDE_FP) || !defined(DISABLE_MQTT)
    if (!fleetProvisioning.Validate())
    {
        return false;
    }
#endif
#if !defined(EXCLUDE_PUBSUB) || !defined(DISABLE_MQTT)
    if (!pubSub.Validate())
    {
        return false;
    }
#endif
#if !defined(EXCLUDE_SHADOW) || !defined(DISABLE_MQTT)
    if (!sampleShadow.Validate() || !configShadow.Validate())
    {
        return false;
    }
#endif
    if (secureElement.enabled && !secureElement.Validate())
    {
        return false;
    }
#if !defined(EXCLUDE_SENSOR_PUBLISH) || !defined(DISABLE_MQTT)
    if (!sensorPublish.Validate())
    {
        return false;
    }
#endif

    return true;
}

void PlainConfig::SerializeToObject(Crt::JsonObject &object) const
{
    if (endpoint.has_value() && endpoint->c_str())
    {
        object.WithString(JSON_KEY_ENDPOINT, endpoint->c_str());
    }
    if (cert.has_value() && cert->c_str())
    {
        object.WithString(JSON_KEY_CERT, cert->c_str());
    }
    if (key.has_value() && key->c_str())
    {
        object.WithString(JSON_KEY_KEY, key->c_str());
    }
    if (rootCa.has_value() && rootCa->c_str())
    {
        object.WithString(JSON_KEY_ROOT_CA, rootCa->c_str());
    }
    if (thingName.has_value() && thingName->c_str())
    {
        object.WithString(JSON_KEY_THING_NAME, thingName->c_str());
    }

    Crt::JsonObject loggingObject;
    logConfig.SerializeToObject(loggingObject);
    object.WithObject(JSON_KEY_LOGGING, loggingObject);

    Crt::JsonObject jobsObject;
    jobs.SerializeToObject(jobsObject);
    object.WithObject(JSON_KEY_JOBS, jobsObject);

    Crt::JsonObject tunnelingObject;
    tunneling.SerializeToObject(tunnelingObject);
    object.WithObject(JSON_KEY_TUNNELING, tunnelingObject);

    Crt::JsonObject deviceDefenderObject;
    deviceDefender.SerializeToObject(deviceDefenderObject);
    object.WithObject(JSON_KEY_DEVICE_DEFENDER, deviceDefenderObject);

    Crt::JsonObject fleetProvisioningObject;
    fleetProvisioning.SerializeToObject(fleetProvisioningObject);
    object.WithObject(JSON_KEY_FLEET_PROVISIONING, fleetProvisioningObject);

    if (fleetProvisioning.enabled)
    {
        Crt::JsonObject fleetProvisioningRuntimeObject;
        fleetProvisioningRuntimeConfig.SerializeToObject(fleetProvisioningRuntimeObject);
        object.WithObject(JSON_KEY_RUNTIME_CONFIG, fleetProvisioningRuntimeObject);
    }

    Crt::JsonObject samplesObject;
    Crt::JsonObject pubSubObject;
    pubSub.SerializeToObject(pubSubObject);
    samplesObject.WithObject(JSON_KEY_PUB_SUB, pubSubObject);
    object.WithObject(JSON_KEY_SAMPLES, samplesObject);

    Crt::JsonObject configShadowObject;
    configShadow.SerializeToObject(configShadowObject);
    object.WithObject(JSON_KEY_CONFIG_SHADOW, configShadowObject);

    Crt::JsonObject sampleShadowObject;
    sampleShadow.SampleShadow::SerializeToObject(sampleShadowObject);
    object.WithObject(JSON_KEY_SAMPLE_SHADOW, sampleShadowObject);

    Crt::JsonObject secureElementObject;
    secureElement.SerializeToObject(secureElementObject);
    object.WithObject(JSON_KEY_SECURE_ELEMENT, secureElementObject);

    if (sensorPublish.enabled)
    {
        Crt::JsonObject sensorPublishObject;
        sensorPublish.SerializeToObject(sensorPublishObject);
        object.WithObject(JSON_KEY_SENSOR_PUBLISH, sensorPublishObject);
    }
}

constexpr char PlainConfig::LogConfig::LOG_TYPE_FILE[];
constexpr char PlainConfig::LogConfig::LOG_TYPE_STDOUT[];

constexpr char PlainConfig::LogConfig::CLI_LOG_LEVEL[];
constexpr char PlainConfig::LogConfig::CLI_LOG_TYPE[];
constexpr char PlainConfig::LogConfig::CLI_LOG_FILE[];

constexpr char PlainConfig::LogConfig::JSON_KEY_LOG_LEVEL[];
constexpr char PlainConfig::LogConfig::JSON_KEY_LOG_TYPE[];
constexpr char PlainConfig::LogConfig::JSON_KEY_LOG_FILE[];

constexpr char PlainConfig::LogConfig::CLI_ENABLE_SDK_LOGGING[];
constexpr char PlainConfig::LogConfig::CLI_SDK_LOG_LEVEL[];
constexpr char PlainConfig::LogConfig::CLI_SDK_LOG_FILE[];

constexpr char PlainConfig::LogConfig::JSON_KEY_ENABLE_SDK_LOGGING[];
constexpr char PlainConfig::LogConfig::JSON_KEY_SDK_LOG_LEVEL[];
constexpr char PlainConfig::LogConfig::JSON_KEY_SDK_LOG_FILE[];

int PlainConfig::LogConfig::ParseDeviceClientLogLevel(const string &level) const
{
    string temp = level;
    std::transform(temp.begin(), temp.end(), temp.begin(), [](unsigned char c) { return std::toupper(c); });

    if ("DEBUG" == temp)
    {
        return (int)Aws::Iot::DeviceClient::Logging::LogLevel::DEBUG;
    }
    else if ("INFO" == temp)
    {
        return (int)Aws::Iot::DeviceClient::Logging::LogLevel::INFO;
    }
    else if ("WARN" == temp)
    {
        return (int)Aws::Iot::DeviceClient::Logging::LogLevel::WARN;
    }
    else if ("ERROR" == temp)
    {
        return (int)Aws::Iot::DeviceClient::Logging::LogLevel::ERROR;
    }
    else
    {
        throw std::invalid_argument(FormatMessage(
            "Provided log level %s is not a known log level for the AWS IoT Device Client", Sanitize(level).c_str()));
    }
}

Aws::Crt::LogLevel PlainConfig::LogConfig::ParseSDKLogLevel(const string &level) const
{
    string temp = level;
    std::transform(temp.begin(), temp.end(), temp.begin(), [](unsigned char c) { return std::toupper(c); });

    if ("TRACE" == temp)
    {
        return Aws::Crt::LogLevel::Trace;
    }
    else if ("DEBUG" == temp)
    {
        return Aws::Crt::LogLevel::Debug;
    }
    else if ("INFO" == temp)
    {
        return Aws::Crt::LogLevel::Info;
    }
    else if ("WARN" == temp)
    {
        return Aws::Crt::LogLevel::Warn;
    }
    else if ("ERROR" == temp)
    {
        return Aws::Crt::LogLevel::Error;
    }
    else if ("FATAL" == temp)
    {
        return Aws::Crt::LogLevel::Fatal;
    }
    else
    {
        throw std::invalid_argument(
            FormatMessage("Provided log level %s is not a known log level for the SDK", Sanitize(level).c_str()));
    }
}

string PlainConfig::LogConfig::ParseDeviceClientLogType(const string &value) const
{
    string temp = value;
    // Convert to lowercase for comparisons
    std::transform(temp.begin(), temp.end(), temp.begin(), [](unsigned char c) { return std::tolower(c); });
    if (LOG_TYPE_FILE == temp)
    {
        return LOG_TYPE_FILE;
    }
    else if (LOG_TYPE_STDOUT == temp)
    {
        return LOG_TYPE_STDOUT;
    }
    else
    {
        throw std::invalid_argument(FormatMessage(
            "Provided log type %s is not a known log type. Acceptable values are: [%s, %s]",
            Sanitize(value).c_str(),
            LOG_TYPE_FILE,
            LOG_TYPE_STDOUT));
    }
}

string PlainConfig::LogConfig::StringifyDeviceClientLogLevel(int level) const
{

    switch (static_cast<DeviceClient::Logging::LogLevel>(level))
    {
        case DeviceClient::Logging::LogLevel::ERROR:
            return "ERROR";
        case DeviceClient::Logging::LogLevel::WARN:
            return "WARN";
        case DeviceClient::Logging::LogLevel::INFO:
            return "INFO";
        case DeviceClient::Logging::LogLevel::DEBUG:
            return "DEBUG";
    }
    throw std::invalid_argument(FormatMessage("Provided log level, %d is not known", level));
}

string PlainConfig::LogConfig::StringifySDKLogLevel(Aws::Crt::LogLevel level) const
{
    const char *levelString;
    aws_log_level_to_string(static_cast<aws_log_level>(level), &levelString);
    return levelString;
}

bool PlainConfig::LogConfig::LoadFromJson(const Crt::JsonView &json)
{
    const char *jsonKey = JSON_KEY_LOG_LEVEL;
    if (json.ValueExists(jsonKey))
    {
        if (!json.GetString(jsonKey).empty())
        {
            try
            {
                deviceClientlogLevel = ParseDeviceClientLogLevel(json.GetString(jsonKey).c_str());
            }
            catch (const std::invalid_argument &e)
            {
                LOGM_ERROR(Config::TAG, "Unable to parse incoming log level value passed via JSON: %s", e.what());
                return false;
            }
        }
        else
        {
            LOGM_WARN(Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
        }
    }

    jsonKey = JSON_KEY_LOG_TYPE;
    if (json.ValueExists(jsonKey))
    {
        if (!json.GetString(jsonKey).empty())
        {
            try
            {
                deviceClientLogtype = ParseDeviceClientLogType(json.GetString(jsonKey).c_str());
            }
            catch (const std::invalid_argument &e)
            {
                LOGM_ERROR(Config::TAG, "Unable to parse incoming log type value passed via JSON: %s", e.what());
                return false;
            }
        }
        else
        {
            LOGM_WARN(Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
        }
    }

    jsonKey = JSON_KEY_LOG_FILE;
    if ((deviceClientLogtype == LOG_TYPE_FILE) && json.ValueExists(jsonKey))
    {
        if (!json.GetString(jsonKey).empty())
        {
            deviceClientLogFile = FileUtils::ExtractExpandedPath(json.GetString(jsonKey).c_str());
        }
        else
        {
            LOGM_WARN(Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
        }
    }

    jsonKey = JSON_KEY_ENABLE_SDK_LOGGING;
    if (json.ValueExists(jsonKey))
    {
        sdkLoggingEnabled = json.GetBool(jsonKey);
    }

    jsonKey = JSON_KEY_SDK_LOG_LEVEL;
    if (json.ValueExists(jsonKey))
    {
        if (!json.GetString(jsonKey).empty())
        {
            try
            {
                sdkLogLevel = ParseSDKLogLevel(json.GetString(jsonKey).c_str());
            }
            catch (const std::invalid_argument &e)
            {
                LOGM_ERROR(Config::TAG, "Unable to parse incoming SDK log type value passed via JSON: %s", e.what());
                return false;
            }
        }
        else
        {
            LOGM_WARN(Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
        }
    }

    jsonKey = JSON_KEY_SDK_LOG_FILE;
    if (json.ValueExists(jsonKey))
    {
        if (!json.GetString(jsonKey).empty())
        {
            sdkLogFile = FileUtils::ExtractExpandedPath(json.GetString(jsonKey).c_str());
        }
        else
        {
            LOGM_WARN(Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
        }
    }

    return true;
}

bool PlainConfig::LogConfig::LoadFromCliArgs(const CliArgs &cliArgs)
{
    if (cliArgs.count(CLI_LOG_LEVEL))
    {
        try
        {
            deviceClientlogLevel = ParseDeviceClientLogLevel(cliArgs.at(CLI_LOG_LEVEL));
        }
        catch (const std::invalid_argument &e)
        {
            LOGM_ERROR(Config::TAG, "Unable to parse incoming log level value passed via command line: %s", e.what());
            return false;
        }
    }

    if (cliArgs.count(CLI_LOG_TYPE))
    {
        try
        {
            deviceClientLogtype = ParseDeviceClientLogType(cliArgs.at(CLI_LOG_TYPE));
        }
        catch (const std::invalid_argument &e)
        {
            LOGM_ERROR(Config::TAG, "Unable to parse incoming log type value passed via command line: %s", e.what());
            return false;
        }
    }

    if (cliArgs.count(CLI_LOG_FILE))
    {
        deviceClientLogFile = FileUtils::ExtractExpandedPath(cliArgs.at(CLI_LOG_FILE).c_str());
    }

    if (cliArgs.count(CLI_ENABLE_SDK_LOGGING))
    {
        sdkLoggingEnabled = true;
    }

    if (cliArgs.count(CLI_SDK_LOG_FILE))
    {
        sdkLogFile = FileUtils::ExtractExpandedPath(cliArgs.at(CLI_SDK_LOG_FILE).c_str());
    }

    if (cliArgs.count(CLI_SDK_LOG_LEVEL))
    {
        try
        {
            sdkLogLevel = ParseSDKLogLevel(cliArgs.at(CLI_SDK_LOG_LEVEL));
        }
        catch (const std::invalid_argument &e)
        {
            LOGM_ERROR(
                Config::TAG, "Unable to parse incoming sdk log level value passed via command line: %s", e.what());
            return false;
        }
    }

    return true;
}

bool PlainConfig::LogConfig::Validate() const
{
    return true;
}

void PlainConfig::LogConfig::SerializeToObject(Crt::JsonObject &object) const
{
    object.WithString(JSON_KEY_LOG_LEVEL, StringifyDeviceClientLogLevel(deviceClientlogLevel).c_str());
    object.WithString(JSON_KEY_LOG_TYPE, deviceClientLogtype.c_str());
    object.WithString(JSON_KEY_LOG_FILE, deviceClientLogFile.c_str());
    object.WithBool(JSON_KEY_ENABLE_SDK_LOGGING, sdkLoggingEnabled);
    object.WithString(JSON_KEY_SDK_LOG_LEVEL, StringifySDKLogLevel(sdkLogLevel).c_str());
    object.WithString(JSON_KEY_SDK_LOG_FILE, sdkLogFile.c_str());
}

constexpr char PlainConfig::Jobs::CLI_ENABLE_JOBS[];
constexpr char PlainConfig::Jobs::CLI_HANDLER_DIR[];
constexpr char PlainConfig::Jobs::JSON_KEY_ENABLED[];
constexpr char PlainConfig::Jobs::JSON_KEY_HANDLER_DIR[];

bool PlainConfig::Jobs::LoadFromJson(const Crt::JsonView &json)
{
    const char *jsonKey = JSON_KEY_ENABLED;
    if (json.ValueExists(jsonKey))
    {
        enabled = json.GetBool(jsonKey);
    }

    jsonKey = JSON_KEY_HANDLER_DIR;
    if (json.ValueExists(jsonKey) && !json.GetString(jsonKey).empty())
    {
        handlerDir = FileUtils::ExtractExpandedPath(json.GetString(jsonKey).c_str());
    }

    return true;
}

bool PlainConfig::Jobs::LoadFromCliArgs(const CliArgs &cliArgs)
{
    if (cliArgs.count(PlainConfig::Jobs::CLI_ENABLE_JOBS))
    {
        enabled = cliArgs.at(CLI_ENABLE_JOBS).compare("true") == 0;
    }

    if (cliArgs.count(PlainConfig::Jobs::CLI_HANDLER_DIR))
    {
        handlerDir = FileUtils::ExtractExpandedPath(cliArgs.at(CLI_HANDLER_DIR).c_str());
    }

    return true;
}

bool PlainConfig::Jobs::Validate() const
{
    return true;
}

void PlainConfig::Jobs::SerializeToObject(Crt::JsonObject &object) const
{

    object.WithBool(JSON_KEY_ENABLED, enabled);

    if (handlerDir.c_str())
    {
        object.WithString(JSON_KEY_HANDLER_DIR, handlerDir.c_str());
    }
}

constexpr char PlainConfig::Tunneling::CLI_ENABLE_TUNNELING[];
constexpr char PlainConfig::Tunneling::CLI_TUNNELING_DISABLE_NOTIFICATION[];
constexpr char PlainConfig::Tunneling::CLI_TUNNELING_REGION[];
constexpr char PlainConfig::Tunneling::CLI_TUNNELING_SERVICE[];
constexpr char PlainConfig::Tunneling::JSON_KEY_ENABLED[];
constexpr char PlainConfig::Tunneling::JSON_KEY_ENDPOINT[];

bool PlainConfig::Tunneling::LoadFromJson(const Crt::JsonView &json)
{
    const char *jsonKey = JSON_KEY_ENABLED;
    if (json.ValueExists(jsonKey))
    {
        enabled = json.GetBool(jsonKey);
    }

    jsonKey = JSON_KEY_ENDPOINT;
    if (json.ValueExists(jsonKey))
    {
        endpoint = json.GetString(jsonKey).c_str();
    }

    return true;
}

bool PlainConfig::Tunneling::LoadFromCliArgs(const CliArgs &cliArgs)
{
    if (cliArgs.count(PlainConfig::Tunneling::CLI_ENABLE_TUNNELING))
    {
        enabled = cliArgs.at(CLI_ENABLE_TUNNELING).compare("true") == 0;
    }
    if (cliArgs.count(PlainConfig::Tunneling::CLI_TUNNELING_DISABLE_NOTIFICATION))
    {
        subscribeNotification = false;
    }
    if (cliArgs.count(PlainConfig::Tunneling::CLI_TUNNELING_REGION))
    {
        region = cliArgs.at(PlainConfig::Tunneling::CLI_TUNNELING_REGION).c_str();
    }
    if (cliArgs.count(PlainConfig::Tunneling::CLI_TUNNELING_SERVICE))
    {
#if !defined(EXCLUDE_ST)
        auto service = cliArgs.at(PlainConfig::Tunneling::CLI_TUNNELING_SERVICE);
        port = SecureTunnelingFeature::GetPortFromService(service);
#else
        port = 0;
#endif
    }

    return true;
}

bool PlainConfig::Tunneling::LoadFromEnvironment()
{
    const char *accessToken = std::getenv("AWSIOT_TUNNEL_ACCESS_TOKEN");
    if (accessToken)
    {
        destinationAccessToken = accessToken;
    }

    return true;
}

bool PlainConfig::Tunneling::Validate() const
{
    if (!enabled)
    {
        return true;
    }
    if (subscribeNotification)
    {
        return true;
    }

    if (!destinationAccessToken.has_value() || destinationAccessToken->empty())
    {
        LOGM_ERROR(Config::TAG, "*** %s: destination-access-token is missing ***", DeviceClient::DC_FATAL_ERROR);
        return false;
    }
    if (!region.has_value() || region->empty())
    {
        LOGM_ERROR(Config::TAG, "*** %s: region is missing ***", DeviceClient::DC_FATAL_ERROR);
        return false;
    }
    if (!port.has_value()
#if !defined(EXCLUDE_ST)
        || !SecureTunnelingFeature::IsValidPort(port.value()))
#else
    )
#endif
    {
        LOGM_ERROR(Config::TAG, "*** %s: port is missing or invalid ***", DeviceClient::DC_FATAL_ERROR);
        return false;
    }

    return true;
}

void PlainConfig::Tunneling::SerializeToObject(Crt::JsonObject &object) const
{
    object.WithBool(JSON_KEY_ENABLED, enabled);
}

constexpr char PlainConfig::DeviceDefender::CLI_ENABLE_DEVICE_DEFENDER[];
constexpr char PlainConfig::DeviceDefender::CLI_DEVICE_DEFENDER_INTERVAL[];

constexpr char PlainConfig::DeviceDefender::JSON_KEY_ENABLED[];
constexpr char PlainConfig::DeviceDefender::JSON_KEY_INTERVAL[];

bool PlainConfig::DeviceDefender::LoadFromJson(const Crt::JsonView &json)
{
    const char *jsonKey = JSON_KEY_ENABLED;
    if (json.ValueExists(jsonKey))
    {
        enabled = json.GetBool(jsonKey);
    }

    jsonKey = JSON_KEY_INTERVAL;
    if (json.ValueExists(jsonKey))
    {
        interval = json.GetInteger(jsonKey);
    }

    return true;
}

bool PlainConfig::DeviceDefender::LoadFromCliArgs(const CliArgs &cliArgs)
{
    if (cliArgs.count(PlainConfig::DeviceDefender::CLI_ENABLE_DEVICE_DEFENDER))
    {
        enabled = cliArgs.at(CLI_ENABLE_DEVICE_DEFENDER).compare("true") == 0;
    }
    if (cliArgs.count(PlainConfig::DeviceDefender::CLI_DEVICE_DEFENDER_INTERVAL))
    {
        try
        {
            interval = stoi(cliArgs.at(PlainConfig::DeviceDefender::CLI_DEVICE_DEFENDER_INTERVAL).c_str());
        }
        catch (const invalid_argument &)
        {
            LOGM_ERROR(
                Config::TAG,
                "*** %s: Failed to convert CLI argument {%s} to integer, please use a "
                "valid integer between 1 and MAX_INT ***",
                DeviceClient::DC_FATAL_ERROR,
                PlainConfig::DeviceDefender::CLI_DEVICE_DEFENDER_INTERVAL);
            return false;
        }
    }
    return true;
}

bool PlainConfig::DeviceDefender::Validate() const
{
    if (!enabled)
    {
        return true;
    }
    if (interval <= 0)
    {
        LOGM_ERROR(Config::TAG, "*** %s: Interval value <= 0 ***", DeviceClient::DC_FATAL_ERROR);
        return false;
    }

    return true;
}

void PlainConfig::DeviceDefender::SerializeToObject(Crt::JsonObject &object) const
{
    object.WithBool(JSON_KEY_ENABLED, enabled);
    object.WithInteger(JSON_KEY_INTERVAL, interval);
}

constexpr char PlainConfig::FleetProvisioning::CLI_ENABLE_FLEET_PROVISIONING[];
constexpr char PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_TEMPLATE_NAME[];
constexpr char PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_TEMPLATE_PARAMETERS[];
constexpr char PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_CSR_FILE[];
constexpr char PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_DEVICE_KEY[];
constexpr char PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_PUBLISH_SYS_INFO[];
constexpr char PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_NETWORK_INTERFACE[];

constexpr char PlainConfig::FleetProvisioning::JSON_KEY_ENABLED[];
constexpr char PlainConfig::FleetProvisioning::JSON_KEY_TEMPLATE_NAME[];
constexpr char PlainConfig::FleetProvisioning::JSON_KEY_TEMPLATE_PARAMETERS[];
constexpr char PlainConfig::FleetProvisioning::JSON_KEY_CSR_FILE[];
constexpr char PlainConfig::FleetProvisioning::JSON_KEY_DEVICE_KEY[];
constexpr char PlainConfig::FleetProvisioning::JSON_KEY_PUBLISH_SYS_INFO[];
constexpr char PlainConfig::FleetProvisioning::JSON_KEY_NETWORK_INTERFACE[];

bool PlainConfig::FleetProvisioning::LoadFromJson(const Crt::JsonView &json)
{
    const char *jsonKey = JSON_KEY_ENABLED;
    if (json.ValueExists(jsonKey))
    {
        enabled = json.GetBool(jsonKey);
    }

    if (enabled)
    {
        jsonKey = JSON_KEY_TEMPLATE_NAME;
        if (json.ValueExists(jsonKey))
        {
            if (!json.GetString(jsonKey).empty())
            {
                templateName = json.GetString(jsonKey).c_str();
            }
            else
            {
                LOGM_WARN(
                    Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
            }
        }

        jsonKey = JSON_KEY_TEMPLATE_PARAMETERS;
        if (json.ValueExists(jsonKey) && !json.GetString(jsonKey).empty())
        {
            templateParameters = json.GetString(jsonKey).c_str();
        }

        jsonKey = JSON_KEY_CSR_FILE;
        if (json.ValueExists(jsonKey))
        {
            if (!json.GetString(jsonKey).empty())
            {
                csrFile = FileUtils::ExtractExpandedPath(json.GetString(jsonKey).c_str());
            }
            else
            {
                LOGM_WARN(
                    Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
            }
        }

        jsonKey = JSON_KEY_DEVICE_KEY;
        if (json.ValueExists(jsonKey))
        {
            if (!json.GetString(jsonKey).empty())
            {
                deviceKey = FileUtils::ExtractExpandedPath(json.GetString(jsonKey).c_str());
            }
            else
            {
                LOGM_WARN(
                    Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
            }
        }

        jsonKey = JSON_KEY_PUBLISH_SYS_INFO;
        if (json.ValueExists(jsonKey))
        {
            collectSystemInformation = json.GetBool(jsonKey);
        }

        jsonKey = JSON_KEY_NETWORK_INTERFACE;
        if (json.ValueExists(jsonKey))
        {
            if (!json.GetString(jsonKey).empty())
            {
                networkInterface = json.GetString(jsonKey).c_str();
            }
            else
            {
                LOGM_WARN(
                    Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
            }
        }
    }

    return true;
}

bool PlainConfig::FleetProvisioning::LoadFromCliArgs(const CliArgs &cliArgs)
{
    if (cliArgs.count(PlainConfig::FleetProvisioning::CLI_ENABLE_FLEET_PROVISIONING))
    {
        enabled = cliArgs.at(CLI_ENABLE_FLEET_PROVISIONING).compare("true") == 0;
    }
    if (cliArgs.count(PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_TEMPLATE_NAME))
    {
        templateName = cliArgs.at(PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_TEMPLATE_NAME).c_str();
    }
    if (cliArgs.count(PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_TEMPLATE_PARAMETERS))
    {
        templateParameters =
            cliArgs.at(PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_TEMPLATE_PARAMETERS).c_str();
    }
    if (cliArgs.count(PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_CSR_FILE))
    {
        csrFile = FileUtils::ExtractExpandedPath(
            cliArgs.at(PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_CSR_FILE).c_str());
    }
    if (cliArgs.count(PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_DEVICE_KEY))
    {
        deviceKey = FileUtils::ExtractExpandedPath(
            cliArgs.at(PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_DEVICE_KEY).c_str());
    }
    if (cliArgs.count(PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_PUBLISH_SYS_INFO))
    {
        enabled = cliArgs.at(CLI_FLEET_PROVISIONING_PUBLISH_SYS_INFO).compare("true") == 0;
    }
    if (cliArgs.count(PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_NETWORK_INTERFACE))
    {
        networkInterface = cliArgs.at(
            PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_NETWORK_INTERFACE).c_str();
    }

    return true;
}

bool PlainConfig::FleetProvisioning::Validate() const
{
    if (!enabled)
    {
        return true;
    }

    if (!templateName.has_value() || templateName->empty())
    {
        LOGM_ERROR(
            Config::TAG,
            "*** %s: A template name must be specified if Fleet Provisioning is enabled "
            "***",
            DeviceClient::DC_FATAL_ERROR);
        return false;
    }

    if (csrFile.has_value() && !csrFile->empty() && !FileUtils::IsValidFilePath(csrFile->c_str()))
    {
        return false;
    }

    if (deviceKey.has_value() && !deviceKey->empty() && !FileUtils::IsValidFilePath(deviceKey->c_str()))
    {
        return false;
    }

    return true;
}

void PlainConfig::FleetProvisioning::SerializeToObject(Crt::JsonObject &object) const
{
    object.WithBool(JSON_KEY_ENABLED, enabled);

    if (templateName.has_value() && templateName->c_str())
    {
        object.WithString(JSON_KEY_TEMPLATE_NAME, templateName->c_str());
    }

    if (templateParameters.has_value() && templateParameters->c_str())
    {
        object.WithString(JSON_KEY_TEMPLATE_PARAMETERS, templateParameters->c_str());
    }

    if (csrFile.has_value() && csrFile->c_str())
    {
        object.WithString(JSON_KEY_CSR_FILE, csrFile->c_str());
    }

    if (deviceKey.has_value() && deviceKey->c_str())
    {
        object.WithString(JSON_KEY_DEVICE_KEY, deviceKey->c_str());
    }

    if (collectSystemInformation)
    {
        object.WithBool(JSON_KEY_PUBLISH_SYS_INFO, true);
    }

    if (networkInterface.has_value() && networkInterface->c_str())
    {
        object.WithString(JSON_KEY_NETWORK_INTERFACE, networkInterface->c_str());
    }
}

constexpr char PlainConfig::FleetProvisioningRuntimeConfig::JSON_KEY_COMPLETED_FLEET_PROVISIONING[];
constexpr char PlainConfig::FleetProvisioningRuntimeConfig::JSON_KEY_CERT[];
constexpr char PlainConfig::FleetProvisioningRuntimeConfig::JSON_KEY_KEY[];
constexpr char PlainConfig::FleetProvisioningRuntimeConfig::JSON_KEY_THING_NAME[];
constexpr char PlainConfig::FleetProvisioningRuntimeConfig::JSON_KEY_DEVICE_CONFIG[];

bool PlainConfig::FleetProvisioningRuntimeConfig::LoadFromJson(const Crt::JsonView &json)
{
    const char *jsonKey = JSON_KEY_COMPLETED_FLEET_PROVISIONING;
    if (json.ValueExists(jsonKey))
    {
        completedFleetProvisioning = json.GetBool(jsonKey);
    }

    if (completedFleetProvisioning)
    {
        jsonKey = JSON_KEY_CERT;
        if (json.ValueExists(jsonKey))
        {
            if (!json.GetString(jsonKey).empty())
            {
                cert = FileUtils::ExtractExpandedPath(json.GetString(jsonKey).c_str());
            }
            else
            {
                LOGM_WARN(
                    Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
            }
        }

        jsonKey = JSON_KEY_KEY;
        if (json.ValueExists(jsonKey))
        {
            if (!json.GetString(jsonKey).empty())
            {
                key = FileUtils::ExtractExpandedPath(json.GetString(jsonKey).c_str());
            }
            else
            {
                LOGM_WARN(
                    Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
            }
        }

        jsonKey = JSON_KEY_THING_NAME;
        if (json.ValueExists(jsonKey))
        {
            thingName = json.GetString(jsonKey).c_str();
        }
    }

    return true;
}

bool PlainConfig::FleetProvisioningRuntimeConfig::LoadFromCliArgs(const CliArgs &cliArgs)
{
    /*
     * No Command line arguments for Fleet Provisioning Runtime Config
     */
    return true;
}

bool PlainConfig::FleetProvisioningRuntimeConfig::Validate() const
{
    if (!completedFleetProvisioning)
    {
        return false;
    }
    return cert.has_value() && key.has_value() && thingName.has_value() && !cert->empty() && !key->empty() &&
           !thingName->empty();
}

void PlainConfig::FleetProvisioningRuntimeConfig::SerializeToObject(Aws::Crt::JsonObject &object) const
{
    object.WithBool(JSON_KEY_COMPLETED_FLEET_PROVISIONING, completedFleetProvisioning);

    if (cert.has_value() && cert->c_str())
    {
        object.WithString(JSON_KEY_CERT, cert->c_str());
    }

    if (key.has_value() && key->c_str())
    {
        object.WithString(JSON_KEY_KEY, key->c_str());
    }

    if (thingName.has_value() && thingName->c_str())
    {
        object.WithString(JSON_KEY_THING_NAME, thingName->c_str());
    }
}

constexpr char PlainConfig::HttpProxyConfig::CLI_HTTP_PROXY_CONFIG_PATH[];
constexpr char PlainConfig::HttpProxyConfig::JSON_KEY_HTTP_PROXY_ENABLED[];
constexpr char PlainConfig::HttpProxyConfig::JSON_KEY_HTTP_PROXY_HOST[];
constexpr char PlainConfig::HttpProxyConfig::JSON_KEY_HTTP_PROXY_PORT[];
constexpr char PlainConfig::HttpProxyConfig::JSON_KEY_HTTP_PROXY_AUTH_METHOD[];
constexpr char PlainConfig::HttpProxyConfig::JSON_KEY_HTTP_PROXY_USERNAME[];
constexpr char PlainConfig::HttpProxyConfig::JSON_KEY_HTTP_PROXY_PASSWORD[];

bool PlainConfig::HttpProxyConfig::LoadFromJson(const Crt::JsonView &json)
{
    const char *jsonKey = JSON_KEY_HTTP_PROXY_ENABLED;
    if (json.ValueExists(jsonKey))
    {
        httpProxyEnabled = json.GetBool(jsonKey);
    }

    if (httpProxyEnabled)
    {
        jsonKey = JSON_KEY_HTTP_PROXY_HOST;
        if (json.ValueExists(jsonKey))
        {
            if (!json.GetString(jsonKey).empty())
            {
                proxyHost = json.GetString(jsonKey).c_str();
            }
            else
            {
                LOGM_WARN(
                    Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
            }
        }

        jsonKey = JSON_KEY_HTTP_PROXY_PORT;
        if (json.ValueExists(jsonKey))
        {
            if (!json.GetString(jsonKey).empty())
            {
                try
                {
                    proxyPort = stoi(json.GetString(jsonKey).c_str());
                }
                catch (...)
                {
                    LOGM_ERROR(
                        Config::TAG,
                        "*** %s: Failed to convert JSON key {%s} to integer, please use a "
                        "valid value for port number",
                        DeviceClient::DC_FATAL_ERROR,
                        jsonKey);
                    return false;
                }
            }
            else
            {
                LOGM_WARN(
                    Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
            }
        }

        jsonKey = JSON_KEY_HTTP_PROXY_AUTH_METHOD;
        if (json.ValueExists(jsonKey))
        {
            if (!json.GetString(jsonKey).empty())
            {
                proxyAuthMethod = json.GetString(jsonKey).c_str();
                if (strcmp(proxyAuthMethod->c_str(), "UserNameAndPassword") == 0)
                {
                    httpProxyAuthEnabled = true;
                }
                else if (strcmp(proxyAuthMethod->c_str(), "None") != 0)
                {
                    LOGM_WARN(
                        Config::TAG,
                        "Unrecognized HTTP Proxy Authentication Method value: {%s}. Supported values are "
                        "UserNameAndPassword or None",
                        proxyAuthMethod->c_str());
                }
            }
            else
            {
                LOGM_WARN(
                    Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
            }
        }

        jsonKey = JSON_KEY_HTTP_PROXY_USERNAME;
        if (json.ValueExists(jsonKey))
        {
            if (!json.GetString(jsonKey).empty())
            {
                proxyUsername = json.GetString(jsonKey).c_str();
            }
            else
            {
                LOGM_WARN(
                    Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
            }
        }

        jsonKey = JSON_KEY_HTTP_PROXY_PASSWORD;
        if (json.ValueExists(jsonKey))
        {
            if (!json.GetString(jsonKey).empty())
            {
                proxyPassword = json.GetString(jsonKey).c_str();
            }
            else
            {
                LOGM_WARN(
                    Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
            }
        }
    }
    else
    {
        LOG_INFO(Config::TAG, "HTTP Proxy is disabled as configured.");
    }

    return true;
}

bool PlainConfig::HttpProxyConfig::LoadFromCliArgs(const CliArgs &cliArgs)
{
    if (cliArgs.count(PlainConfig::HttpProxyConfig::CLI_HTTP_PROXY_CONFIG_PATH))
    {
        proxyConfigPath =
            FileUtils::ExtractExpandedPath(cliArgs.at(PlainConfig::HttpProxyConfig::CLI_HTTP_PROXY_CONFIG_PATH))
                .c_str();
    }
    else
    {
        // If http proxy config file path is not provided,
        proxyConfigPath = Config::DEFAULT_HTTP_PROXY_CONFIG_FILE;
    }
    return true;
}

bool PlainConfig::HttpProxyConfig::Validate() const
{
    if (!httpProxyEnabled)
    {
        return true;
    }

    if (!proxyHost.has_value() || proxyHost->empty())
    {
        LOGM_ERROR(
            Config::TAG,
            "*** %s: Proxy host name field must be specified if HTTP proxy is enabled ***",
            DeviceClient::DC_FATAL_ERROR);
        return false;
    }

    if (!ProxyUtils::ValidateHostIpAddress(proxyHost->c_str()))
    {
        LOGM_ERROR(
            Config::TAG,
            "*** %s: Proxy host IP address must be a private IP address ***",
            DeviceClient::DC_FATAL_ERROR);
        return false;
    }

    if (!proxyPort.has_value() || !ProxyUtils::ValidatePortNumber(proxyPort.value()))
    {
        LOGM_ERROR(
            Config::TAG,
            "*** %s: Valid value of proxy port field must be specified if HTTP proxy is enabled ***",
            DeviceClient::DC_FATAL_ERROR);
        return false;
    }

    if (!proxyAuthMethod.has_value() || proxyAuthMethod->empty())
    {
        LOGM_ERROR(
            Config::TAG,
            "*** %s: Proxy auth method field must be specified if HTTP proxy is enabled ***",
            DeviceClient::DC_FATAL_ERROR);
        return true;
    }

    if (httpProxyAuthEnabled && (!proxyUsername.has_value() || proxyUsername->empty()))
    {
        LOGM_ERROR(
            Config::TAG,
            "*** %s: Proxy username field must be specified if HTTP proxy authentication is enabled ***",
            DeviceClient::DC_FATAL_ERROR);
        return false;
    }

    if (httpProxyAuthEnabled && (!proxyPassword.has_value() || proxyPassword->empty()))
    {
        LOGM_ERROR(
            Config::TAG,
            "*** %s: Proxy password field must be specified if HTTP proxy authentication is enabled ***",
            DeviceClient::DC_FATAL_ERROR);
        return false;
    }

    return true;
}

constexpr char PlainConfig::PubSub::CLI_ENABLE_PUB_SUB[];
constexpr char PlainConfig::PubSub::CLI_PUB_SUB_PUBLISH_TOPIC[];
constexpr char PlainConfig::PubSub::CLI_PUB_SUB_PUBLISH_FILE[];
constexpr char PlainConfig::PubSub::CLI_PUB_SUB_SUBSCRIBE_TOPIC[];
constexpr char PlainConfig::PubSub::CLI_PUB_SUB_SUBSCRIBE_FILE[];

constexpr char PlainConfig::PubSub::JSON_ENABLE_PUB_SUB[];
constexpr char PlainConfig::PubSub::JSON_PUB_SUB_PUBLISH_TOPIC[];
constexpr char PlainConfig::PubSub::JSON_PUB_SUB_PUBLISH_FILE[];
constexpr char PlainConfig::PubSub::JSON_PUB_SUB_SUBSCRIBE_TOPIC[];
constexpr char PlainConfig::PubSub::JSON_PUB_SUB_SUBSCRIBE_FILE[];
constexpr char PlainConfig::PubSub::JSON_PUB_SUB_PUBLISH_ON_CHANGE[];

bool PlainConfig::PubSub::LoadFromJson(const Crt::JsonView &json)
{
    const char *jsonKey = JSON_ENABLE_PUB_SUB;
    if (json.ValueExists(jsonKey))
    {
        enabled = json.GetBool(jsonKey);
    }

    if (enabled)
    {
        jsonKey = JSON_PUB_SUB_PUBLISH_TOPIC;
        if (json.ValueExists(jsonKey))
        {
            if (!json.GetString(jsonKey).empty())
            {
                publishTopic = json.GetString(jsonKey).c_str();
            }
            else
            {
                LOGM_WARN(
                    Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
            }
        }

        jsonKey = JSON_PUB_SUB_PUBLISH_FILE;
        if (json.ValueExists(jsonKey))
        {
            if (!json.GetString(jsonKey).empty())
            {
                publishFile = json.GetString(jsonKey).c_str();
            }
            else
            {
                LOGM_WARN(
                    Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
            }
        }

        jsonKey = JSON_PUB_SUB_SUBSCRIBE_TOPIC;
        if (json.ValueExists(jsonKey))
        {
            if (!json.GetString(jsonKey).empty())
            {
                subscribeTopic = json.GetString(jsonKey).c_str();
            }
            else
            {
                LOGM_WARN(
                    Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
            }
        }

        jsonKey = JSON_PUB_SUB_SUBSCRIBE_FILE;
        if (json.ValueExists(jsonKey))
        {
            if (!json.GetString(jsonKey).empty())
            {
                subscribeFile = FileUtils::ExtractExpandedPath(json.GetString(jsonKey).c_str());
            }
            else
            {
                LOGM_WARN(
                    Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
            }
        }
    }

    jsonKey = JSON_PUB_SUB_PUBLISH_ON_CHANGE;
    if (json.ValueExists(jsonKey))
    {
        publishOnChange = json.GetBool(jsonKey);
    }

    return true;
}

bool PlainConfig::PubSub::LoadFromCliArgs(const CliArgs &cliArgs)
{
    if (cliArgs.count(PlainConfig::PubSub::CLI_ENABLE_PUB_SUB))
    {
        enabled = cliArgs.at(CLI_ENABLE_PUB_SUB).compare("true") == 0;
    }
    if (cliArgs.count(PlainConfig::PubSub::CLI_PUB_SUB_PUBLISH_TOPIC))
    {
        publishTopic = cliArgs.at(PlainConfig::PubSub::CLI_PUB_SUB_PUBLISH_TOPIC);
    }
    if (cliArgs.count(PlainConfig::PubSub::CLI_PUB_SUB_PUBLISH_FILE))
    {
        publishFile = cliArgs.at(PlainConfig::PubSub::CLI_PUB_SUB_PUBLISH_FILE);
    }
    if (cliArgs.count(PlainConfig::PubSub::CLI_PUB_SUB_SUBSCRIBE_TOPIC))
    {
        subscribeTopic = cliArgs.at(PlainConfig::PubSub::CLI_PUB_SUB_SUBSCRIBE_TOPIC);
    }
    if (cliArgs.count(PlainConfig::PubSub::CLI_PUB_SUB_SUBSCRIBE_FILE))
    {
        subscribeFile = FileUtils::ExtractExpandedPath(cliArgs.at(PlainConfig::PubSub::CLI_PUB_SUB_SUBSCRIBE_FILE));
    }
    return true;
}

bool PlainConfig::PubSub::Validate() const
{
    if (!enabled)
    {
        return true;
    }
    if (!publishTopic.has_value() || publishTopic->empty())
    {
        LOGM_ERROR(
            Config::TAG,
            "*** %s: Publish Topic field must be specified if Pub-Sub sample feature is enabled ***",
            DeviceClient::DC_FATAL_ERROR);
        return false;
    }

    if (!subscribeTopic.has_value() || subscribeTopic->empty())
    {
        LOGM_ERROR(
            Config::TAG,
            "*** %s: Subscribe Topic field must be specified if Pub-Sub sample feature is enabled ***",
            DeviceClient::DC_FATAL_ERROR);
        return false;
    }

    return true;
}

void PlainConfig::PubSub::SerializeToObject(Crt::JsonObject &object) const
{
    object.WithBool(JSON_ENABLE_PUB_SUB, enabled);

    if (publishTopic.has_value() && publishTopic->c_str())
    {
        object.WithString(JSON_PUB_SUB_PUBLISH_TOPIC, publishTopic->c_str());
    }

    if (publishFile.has_value() && publishFile->c_str())
    {
        object.WithString(JSON_PUB_SUB_PUBLISH_FILE, publishFile->c_str());
    }

    if (subscribeTopic.has_value() && subscribeTopic->c_str())
    {
        object.WithString(JSON_PUB_SUB_SUBSCRIBE_TOPIC, subscribeTopic->c_str());
    }

    if (subscribeFile.has_value() && subscribeFile->c_str())
    {
        object.WithString(JSON_PUB_SUB_SUBSCRIBE_FILE, subscribeFile->c_str());
    }
}

constexpr char PlainConfig::SampleShadow::CLI_ENABLE_SAMPLE_SHADOW[];
constexpr char PlainConfig::SampleShadow::CLI_SAMPLE_SHADOW_NAME[];
constexpr char PlainConfig::SampleShadow::CLI_SAMPLE_SHADOW_INPUT_FILE[];
constexpr char PlainConfig::SampleShadow::CLI_SAMPLE_SHADOW_OUTPUT_FILE[];

constexpr char PlainConfig::SampleShadow::JSON_ENABLE_SAMPLE_SHADOW[];
constexpr char PlainConfig::SampleShadow::JSON_SAMPLE_SHADOW_NAME[];
constexpr char PlainConfig::SampleShadow::JSON_SAMPLE_SHADOW_INPUT_FILE[];
constexpr char PlainConfig::SampleShadow::JSON_SAMPLE_SHADOW_OUTPUT_FILE[];

bool PlainConfig::SampleShadow::createShadowOutputFile()
{
    if (!FileUtils::CreateDirectoryWithPermissions(Config::DEFAULT_SAMPLE_SHADOW_OUTPUT_DIR, S_IRWXU))
    {
        LOGM_ERROR(
            Config::TAG,
            "Failed to access/create default directories: %s required for storage of shadow document",
            Config::DEFAULT_SAMPLE_SHADOW_OUTPUT_DIR);

        return false;
    }
    else
    {
        ostringstream outputPathStream;
        outputPathStream << Config::DEFAULT_SAMPLE_SHADOW_OUTPUT_DIR << Config::DEFAULT_SAMPLE_SHADOW_DOCUMENT_FILE;
        LOG_INFO(Config::TAG, outputPathStream.str().c_str());

        string outputPath = FileUtils::ExtractExpandedPath(outputPathStream.str().c_str());

        if (FileUtils::StoreValueInFile("", outputPath))
        {
            chmod(outputPath.c_str(), S_IRUSR | S_IWUSR);
            if (FileUtils::ValidateFilePermissions(outputPath.c_str(), Permissions::SAMPLE_SHADOW_FILES))
            {
                shadowOutputFile = outputPath;
                LOGM_INFO(
                    Config::TAG,
                    "Succesfully create default file: %s required for storage of shadow document",
                    outputPath.c_str());
            }
        }
        else
        {
            LOGM_ERROR(
                Config::TAG,
                "Failed to access/create default file: %s required for storage of shadow document",
                outputPath.c_str());

            return false;
        }
    }

    return true;
}

bool PlainConfig::SampleShadow::LoadFromJson(const Crt::JsonView &json)
{
    const char *jsonKey = JSON_ENABLE_SAMPLE_SHADOW;
    if (json.ValueExists(jsonKey))
    {
        enabled = json.GetBool(jsonKey);
    }

    if (enabled)
    {
        jsonKey = JSON_SAMPLE_SHADOW_NAME;
        if (!json.GetString(jsonKey).empty())
        {
            shadowName = json.GetString(jsonKey).c_str();
        }
        else
        {
            LOGM_WARN(
                Config::TAG,
                "Shadow Name {%s} was provided in the JSON configuration file with an empty value",
                jsonKey);
        }

        jsonKey = JSON_SAMPLE_SHADOW_INPUT_FILE;
        if (json.ValueExists(jsonKey))
        {
            if (!json.GetString(jsonKey).empty())
            {
                shadowInputFile = FileUtils::ExtractExpandedPath(json.GetString(jsonKey).c_str());
            }
            else
            {
                LOGM_WARN(
                    Config::TAG,
                    "Input file {%s} was provided in the JSON configuration file with an empty value",
                    jsonKey);
            }
        }

        jsonKey = JSON_SAMPLE_SHADOW_OUTPUT_FILE;
        if (json.ValueExists(jsonKey) && !json.GetString(jsonKey).empty())
        {
            shadowOutputFile = FileUtils::ExtractExpandedPath(json.GetString(jsonKey).c_str());
        }
    }

    return true;
}

bool PlainConfig::SampleShadow::LoadFromCliArgs(const CliArgs &cliArgs)
{
    if (cliArgs.count(PlainConfig::SampleShadow::CLI_ENABLE_SAMPLE_SHADOW))
    {
        enabled = cliArgs.at(CLI_ENABLE_SAMPLE_SHADOW).compare("true") == 0;
    }
    if (cliArgs.count(PlainConfig::SampleShadow::CLI_SAMPLE_SHADOW_NAME))
    {
        shadowName = cliArgs.at(PlainConfig::SampleShadow::CLI_SAMPLE_SHADOW_NAME).c_str();
    }
    if (cliArgs.count(PlainConfig::SampleShadow::CLI_SAMPLE_SHADOW_INPUT_FILE))
    {
        shadowInputFile =
            FileUtils::ExtractExpandedPath(cliArgs.at(PlainConfig::SampleShadow::CLI_SAMPLE_SHADOW_INPUT_FILE)).c_str();
    }
    if (cliArgs.count(PlainConfig::SampleShadow::CLI_SAMPLE_SHADOW_OUTPUT_FILE))
    {
        shadowOutputFile =
            FileUtils::ExtractExpandedPath(cliArgs.at(PlainConfig::SampleShadow::CLI_SAMPLE_SHADOW_OUTPUT_FILE))
                .c_str();
    }

    // setting `shadowOutputFile` value to default if no value was passed by user via CLI or JSON config.
    if ((!shadowOutputFile.has_value() || shadowOutputFile->empty()) && !createShadowOutputFile())
    {
        return false;
    }

    return true;
}

bool PlainConfig::SampleShadow::Validate() const
{
    if (!enabled)
    {
        return true;
    }

    if (!shadowName.has_value() || shadowName->empty())
    {
        LOGM_ERROR(
            Config::TAG,
            "*** %s: shadowName field must be specified if Shadow sample feature is enabled ***",
            DeviceClient::DC_FATAL_ERROR);
        return false;
    }

    if (shadowInputFile.has_value() && !shadowInputFile->empty())
    {
        if (FileUtils::IsValidFilePath(shadowInputFile->c_str()))
        {
            if (!FileUtils::ValidateFilePermissions(shadowInputFile.value(), Permissions::SAMPLE_SHADOW_FILES, true))
            {
                return false;
            }
        }
        else
        {
            LOGM_ERROR(
                Config::TAG,
                "*** %s: Invalid file path {%s} passed for argument: %s ***",
                DeviceClient::DC_FATAL_ERROR,
                shadowInputFile.value().c_str(),
                JSON_SAMPLE_SHADOW_INPUT_FILE);
            return false;
        }

        size_t incomingFileSize = FileUtils::GetFileSize(shadowInputFile->c_str());
        if (MAXIMUM_SHADOW_INPUT_FILE_SIZE < incomingFileSize)
        {
            LOGM_ERROR(
                Config::TAG,
                "Refusing to open input file %s, file size %zu bytes is greater than allowable limit of %zu bytes",
                Sanitize(shadowInputFile->c_str()).c_str(),
                incomingFileSize,
                MAXIMUM_SHADOW_INPUT_FILE_SIZE);
            return false;
        }
    }

    if (shadowOutputFile.has_value() && !shadowOutputFile->empty())
    {
        if (FileUtils::IsValidFilePath(shadowOutputFile->c_str()))
        {
            if (!FileUtils::ValidateFilePermissions(shadowOutputFile.value(), Permissions::SAMPLE_SHADOW_FILES, true))
            {
                return false;
            }
        }
        else
        {
            LOGM_ERROR(
                Config::TAG,
                "*** %s: Invalid file path {%s} passed for argument: %s ***",
                DeviceClient::DC_FATAL_ERROR,
                shadowOutputFile.value().c_str(),
                JSON_SAMPLE_SHADOW_OUTPUT_FILE);
            return false;
        }
    }

    return true;
}

void PlainConfig::SampleShadow::SerializeToObject(Crt::JsonObject &object) const
{

    object.WithBool(JSON_ENABLE_SAMPLE_SHADOW, enabled);

    if (shadowName.has_value() && shadowName->c_str())
    {
        object.WithString(JSON_SAMPLE_SHADOW_NAME, shadowName->c_str());
    }

    if (shadowInputFile.has_value() && shadowInputFile->c_str())
    {
        object.WithString(JSON_SAMPLE_SHADOW_INPUT_FILE, shadowInputFile->c_str());
    }

    if (shadowOutputFile.has_value() && shadowOutputFile->c_str())
    {
        object.WithString(JSON_SAMPLE_SHADOW_OUTPUT_FILE, shadowOutputFile->c_str());
    }
}

constexpr char PlainConfig::ConfigShadow::JSON_ENABLE_CONFIG_SHADOW[];
constexpr char PlainConfig::ConfigShadow::CLI_ENABLE_CONFIG_SHADOW[];

bool PlainConfig::ConfigShadow::LoadFromJson(const Crt::JsonView &json)
{
    const char *jsonKey = JSON_ENABLE_CONFIG_SHADOW;
    if (json.ValueExists(jsonKey))
    {
        enabled = json.GetBool(jsonKey);
    }

    return true;
}

bool PlainConfig::ConfigShadow::LoadFromCliArgs(const CliArgs &cliArgs)
{
    if (cliArgs.count(PlainConfig::ConfigShadow::CLI_ENABLE_CONFIG_SHADOW))
    {
        enabled = cliArgs.at(CLI_ENABLE_CONFIG_SHADOW).compare("true") == 0;
    }

    return true;
}

bool PlainConfig::ConfigShadow::Validate() const
{
    return true;
}

void PlainConfig::ConfigShadow::SerializeToObject(Crt::JsonObject &object) const
{
    object.WithBool(JSON_ENABLE_CONFIG_SHADOW, enabled);
}

constexpr char PlainConfig::SecureElement::CLI_ENABLE_SECURE_ELEMENT[];
constexpr char PlainConfig::SecureElement::CLI_PKCS11_LIB[];
constexpr char PlainConfig::SecureElement::CLI_SECURE_ELEMENT_PIN[];
constexpr char PlainConfig::SecureElement::CLI_SECURE_ELEMENT_KEY_LABEL[];
constexpr char PlainConfig::SecureElement::CLI_SECURE_ELEMENT_SLOT_ID[];
constexpr char PlainConfig::SecureElement::CLI_SECURE_ELEMENT_TOKEN_LABEL[];

constexpr char PlainConfig::SecureElement::JSON_ENABLE_SECURE_ELEMENT[];
constexpr char PlainConfig::SecureElement::JSON_PKCS11_LIB[];
constexpr char PlainConfig::SecureElement::JSON_SECURE_ELEMENT_PIN[];
constexpr char PlainConfig::SecureElement::JSON_SECURE_ELEMENT_KEY_LABEL[];
constexpr char PlainConfig::SecureElement::JSON_SECURE_ELEMENT_SLOT_ID[];
constexpr char PlainConfig::SecureElement::JSON_SECURE_ELEMENT_TOKEN_LABEL[];

bool PlainConfig::SecureElement::LoadFromJson(const Crt::JsonView &json)
{
    const char *jsonKey = JSON_ENABLE_SECURE_ELEMENT;
    if (json.ValueExists(jsonKey))
    {
        enabled = json.GetBool(jsonKey);
    }

    if (enabled)
    {
        jsonKey = JSON_PKCS11_LIB;
        if (json.ValueExists(jsonKey))
        {
            if (!json.GetString(jsonKey).empty())
            {
                pkcs11Lib = FileUtils::ExtractExpandedPath(json.GetString(jsonKey).c_str());
            }
            else
            {
                LOGM_WARN(
                    Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
            }
        }

        jsonKey = JSON_SECURE_ELEMENT_PIN;
        if (json.ValueExists(jsonKey))
        {
            if (!json.GetString(jsonKey).empty())
            {
                secureElementPin = json.GetString(jsonKey).c_str();
            }
            else
            {
                LOGM_WARN(
                    Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
            }
        }

        jsonKey = JSON_SECURE_ELEMENT_KEY_LABEL;
        if (json.ValueExists(jsonKey))
        {
            if (!json.GetString(jsonKey).empty())
            {
                secureElementKeyLabel = json.GetString(jsonKey).c_str();
            }
            else
            {
                LOGM_WARN(
                    Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
            }
        }

        jsonKey = JSON_SECURE_ELEMENT_SLOT_ID;
        if (json.ValueExists(jsonKey))
        {
            if (json.GetInt64(jsonKey))
            {
                secureElementSlotId = json.GetInt64(jsonKey);
            }
            else
            {
                LOGM_WARN(
                    Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
            }
        }

        jsonKey = JSON_SECURE_ELEMENT_TOKEN_LABEL;
        if (json.ValueExists(jsonKey))
        {
            if (!json.GetString(jsonKey).empty())
            {
                secureElementTokenLabel = json.GetString(jsonKey).c_str();
            }
            else
            {
                LOGM_WARN(
                    Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
            }
        }
    }

    return true;
}

bool PlainConfig::SecureElement::LoadFromCliArgs(const CliArgs &cliArgs)
{
    if (cliArgs.count(PlainConfig::SecureElement::CLI_ENABLE_SECURE_ELEMENT))
    {
        enabled = cliArgs.at(PlainConfig::SecureElement::CLI_ENABLE_SECURE_ELEMENT).compare("true") == 0;
    }
    if (cliArgs.count(PlainConfig::SecureElement::CLI_PKCS11_LIB))
    {
        pkcs11Lib = FileUtils::ExtractExpandedPath(cliArgs.at(PlainConfig::SecureElement::CLI_PKCS11_LIB));
    }
    if (cliArgs.count(PlainConfig::SecureElement::CLI_SECURE_ELEMENT_PIN))
    {
        secureElementPin = cliArgs.at(PlainConfig::SecureElement::CLI_SECURE_ELEMENT_PIN);
    }
    if (cliArgs.count(PlainConfig::SecureElement::CLI_SECURE_ELEMENT_KEY_LABEL))
    {
        secureElementKeyLabel = cliArgs.at(PlainConfig::SecureElement::CLI_SECURE_ELEMENT_KEY_LABEL);
    }
    if (cliArgs.count(PlainConfig::SecureElement::CLI_SECURE_ELEMENT_SLOT_ID))
    {
        secureElementSlotId = stoul(cliArgs.at(PlainConfig::SecureElement::CLI_SECURE_ELEMENT_SLOT_ID));
    }
    if (cliArgs.count(PlainConfig::SecureElement::CLI_SECURE_ELEMENT_TOKEN_LABEL))
    {
        secureElementTokenLabel = cliArgs.at(PlainConfig::SecureElement::CLI_SECURE_ELEMENT_TOKEN_LABEL);
    }
    return true;
}

bool PlainConfig::SecureElement::Validate() const
{
    if (!enabled)
    {
        return true;
    }
    if (!pkcs11Lib.has_value() || pkcs11Lib->empty())
    {
        LOGM_ERROR(
            Config::TAG,
            "*** %s: PKCS11 Library path field must be specified if Secure Element Configuration is enabled ***",
            DeviceClient::DC_FATAL_ERROR);
        return false;
    }

    if (!secureElementPin.has_value() || secureElementPin->empty())
    {
        LOGM_ERROR(
            Config::TAG,
            "*** %s: Secure Element Pin field must be specified if Secure Element Configuration is enabled ***",
            DeviceClient::DC_FATAL_ERROR);
        return false;
    }

    return true;
}

void PlainConfig::SecureElement::SerializeToObject(Crt::JsonObject &object) const
{
    object.WithBool(JSON_ENABLE_SECURE_ELEMENT, enabled);

    if (pkcs11Lib.has_value() && pkcs11Lib->c_str())
    {
        object.WithString(JSON_PKCS11_LIB, pkcs11Lib->c_str());
    }

    if (secureElementPin.has_value() && secureElementPin->c_str())
    {
        object.WithString(JSON_SECURE_ELEMENT_PIN, secureElementPin->c_str());
    }

    if (secureElementKeyLabel.has_value() && secureElementKeyLabel->c_str())
    {
        object.WithString(JSON_SECURE_ELEMENT_KEY_LABEL, secureElementKeyLabel->c_str());
    }

    if (secureElementSlotId.has_value())
    {
        object.WithInt64(JSON_SECURE_ELEMENT_SLOT_ID, secureElementSlotId.value());
    }

    if (secureElementTokenLabel.has_value() && secureElementTokenLabel->c_str())
    {
        object.WithString(JSON_SECURE_ELEMENT_TOKEN_LABEL, secureElementTokenLabel->c_str());
    }
}

constexpr char PlainConfig::SensorPublish::JSON_SENSORS[];
constexpr char PlainConfig::SensorPublish::JSON_ENABLED[];
constexpr char PlainConfig::SensorPublish::JSON_NAME[];
constexpr char PlainConfig::SensorPublish::JSON_ADDR[];
constexpr char PlainConfig::SensorPublish::JSON_ADDR_POLL_SEC[];
constexpr char PlainConfig::SensorPublish::JSON_BUFFER_TIME_MS[];
constexpr char PlainConfig::SensorPublish::JSON_BUFFER_SIZE[];
constexpr char PlainConfig::SensorPublish::JSON_BUFFER_CAPACITY[];
constexpr char PlainConfig::SensorPublish::JSON_EOM_DELIMITER[];
constexpr char PlainConfig::SensorPublish::JSON_MQTT_TOPIC[];
constexpr char PlainConfig::SensorPublish::JSON_MQTT_DEAD_LETTER_TOPIC[];
constexpr char PlainConfig::SensorPublish::JSON_MQTT_HEARTBEAT_TOPIC[];
constexpr char PlainConfig::SensorPublish::JSON_HEARTBEAT_TIME_SEC[];

constexpr int64_t PlainConfig::SensorPublish::BUF_CAPACITY_BYTES;
constexpr int64_t PlainConfig::SensorPublish::BUF_CAPACITY_BYTES_MIN;

bool PlainConfig::SensorPublish::LoadFromJson(const Crt::JsonView &json)
{
    const char *sensorsKey = JSON_SENSORS;
    if (json.ValueExists(sensorsKey) && json.GetJsonObject(sensorsKey).IsListType())
    {
        int entryId = 1;
        for (const auto &entry : json.GetArray(sensorsKey))
        {
            SensorPublish::SensorSettings sensorSettings;

            const char *jsonKey = JSON_ENABLED;
            if (entry.ValueExists(jsonKey))
            {
                sensorSettings.enabled = entry.GetBool(jsonKey);
            }

            // If at least one sensor is enabled, then enable the feature.
            if (sensorSettings.enabled)
            {
                enabled = true;
            }

            jsonKey = JSON_NAME;
            if (entry.ValueExists(jsonKey))
            {
                sensorSettings.name = entry.GetString(jsonKey).c_str();
            }
            else
            {
                sensorSettings.name = to_string(entryId);
            }

            jsonKey = JSON_ADDR;
            if (entry.ValueExists(jsonKey))
            {
                sensorSettings.addr = entry.GetString(jsonKey).c_str();
            }

            jsonKey = JSON_ADDR_POLL_SEC;
            if (entry.ValueExists(jsonKey))
            {
                sensorSettings.addrPollSec = entry.GetInt64(jsonKey);
            }

            jsonKey = JSON_BUFFER_TIME_MS;
            if (entry.ValueExists(jsonKey))
            {
                sensorSettings.bufferTimeMs = entry.GetInt64(jsonKey);
            }

            jsonKey = JSON_BUFFER_SIZE;
            if (entry.ValueExists(jsonKey))
            {
                sensorSettings.bufferSize = entry.GetInt64(jsonKey);
            }

            jsonKey = JSON_BUFFER_CAPACITY;
            if (entry.ValueExists(jsonKey))
            {
                sensorSettings.bufferCapacity = entry.GetInt64(jsonKey);
            }

            jsonKey = JSON_EOM_DELIMITER;
            if (entry.ValueExists(jsonKey))
            {
                sensorSettings.eomDelimiter = entry.GetString(jsonKey).c_str();
            }

            jsonKey = JSON_MQTT_TOPIC;
            if (entry.ValueExists(jsonKey))
            {
                sensorSettings.mqttTopic = entry.GetString(jsonKey).c_str();
            }

            jsonKey = JSON_MQTT_DEAD_LETTER_TOPIC;
            if (entry.ValueExists(jsonKey))
            {
                sensorSettings.mqttDeadLetterTopic = entry.GetString(jsonKey).c_str();
            }

            jsonKey = JSON_MQTT_HEARTBEAT_TOPIC;
            if (entry.ValueExists(jsonKey))
            {
                sensorSettings.mqttHeartbeatTopic = entry.GetString(jsonKey).c_str();
            }

            jsonKey = JSON_HEARTBEAT_TIME_SEC;
            if (entry.ValueExists(jsonKey))
            {
                sensorSettings.heartbeatTimeSec = entry.GetInt64(jsonKey);
            }

            settings.push_back(sensorSettings);
            ++entryId;
        }
    }

    return true;
}

bool PlainConfig::SensorPublish::LoadFromCliArgs(const CliArgs &cliArgs)
{
    return true;
}

bool PlainConfig::SensorPublish::LoadFromEnvironment()
{
    return true;
}

bool PlainConfig::SensorPublish::Validate() const
{
    if (settings.empty())
    {
        return true; // Nothing to validate.
    }

    // Check the number of sensor entries in the configuration does not exceed maximum.
    if (settings.size() > MAX_SENSOR_SIZE)
    {
        LOGM_ERROR(
            Config::TAG,
            "*** %s: Number of sensor entries in config (%ld) exceeds maximum (%ld)",
            DeviceClient::DC_FATAL_ERROR,
            settings.size(),
            MAX_SENSOR_SIZE);
        // Disable every sensor entry and disable the feature.
        for (auto &setting : settings)
        {
            setting.enabled = false;
        }
        return false;
    }

    bool atLeastOneValidSensor{false};

    // Validate the settings associated with each sensor.
    // If at least one setting associated with the sensor is invalid, then we disable the sensor.
    for (auto &setting : settings)
    {
        if (!setting.enabled)
        {
            continue; // Skip validation
        }

        // Validate the pathname socket path exists and satisfies permissions.
        if (FileUtils::FileExists(setting.addr.value()))
        {
            // If the path points to an existing file,
            // then check the path satisfies permissions.
            if (!FileUtils::ValidateFilePermissions(setting.addr.value(), Permissions::SENSOR_PUBLISH_ADDR_FILE))
            {
                setting.enabled = false;
            }
        }
        else
        {
            // If the path does not point to an existing file,
            // then check the parent directory exists and has required permissions.
            auto addrParentDir = FileUtils::ExtractParentDirectory(setting.addr.value());
            if (!FileUtils::ValidateFilePermissions(addrParentDir, Permissions::SENSOR_PUBLISH_ADDR_DIR))
            {
                setting.enabled = false;
            }
        }

        // Validate the pathname socket does not exceed max address size.
        // Include extra character for terminating null byte.
        if (setting.addr.value().length() + 1 > AWS_ADDRESS_MAX_LEN)
        {
            setting.enabled = false;
            LOGM_ERROR(
                Config::TAG,
                "*** %s: Config %s length (%ld) exceeds maximum (%ld)",
                DeviceClient::DC_FATAL_ERROR,
                JSON_ADDR,
                setting.addr.value().length() + 1,
                AWS_ADDRESS_MAX_LEN);
        }

        // Validate that mqtt topic name is non-empty.
        if (!setting.mqttTopic.has_value() || setting.mqttTopic.value().empty())
        {
            setting.enabled = false;
            LOGM_ERROR(
                Config::TAG,
                "*** %s: Config %s value must be non-empty",
                DeviceClient::DC_FATAL_ERROR,
                JSON_MQTT_TOPIC);
        }

        // Validate that mqtt topic names conform to AWS Iot spec.
        if (!MqttUtils::ValidateAwsIotMqttTopicName(setting.mqttTopic.value()))
        {
            setting.enabled = false;
        }
        if (setting.mqttDeadLetterTopic.has_value() && !setting.mqttDeadLetterTopic.value().empty() &&
            !MqttUtils::ValidateAwsIotMqttTopicName(setting.mqttDeadLetterTopic.value()))
        {
            setting.enabled = false;
        }
        if (setting.mqttHeartbeatTopic.has_value() && !setting.mqttHeartbeatTopic.value().empty() &&
            !MqttUtils::ValidateAwsIotMqttTopicName(setting.mqttHeartbeatTopic.value()))
        {
            setting.enabled = false;
        }

        // Validate that delimiter is non-empty and valid.
        if (!setting.eomDelimiter.has_value() || setting.eomDelimiter.value().empty())
        {
            setting.enabled = false;
            LOGM_ERROR(
                Config::TAG,
                "*** %s: Config %s value must be non-empty",
                DeviceClient::DC_FATAL_ERROR,
                JSON_EOM_DELIMITER);
        }
        else
        {
            // Validate the regular expression by checking for exceptions when compiling the pattern.
            try
            {
                std::regex re(setting.eomDelimiter.value());
            }
            catch (const std::regex_error &e)
            {
                setting.enabled = false;
                LOGM_ERROR(
                    Config::TAG,
                    "*** %s: Config %s value is not a valid regular expression: %s",
                    DeviceClient::DC_FATAL_ERROR,
                    JSON_EOM_DELIMITER,
                    e.what());
            }
        }

        // Validate that numeric values are non-negative.
        if (setting.addrPollSec.value() < 0)
        {
            setting.enabled = false;
            LOGM_ERROR(
                Config::TAG,
                "*** %s: Config %s value %ld must be non-negative",
                DeviceClient::DC_FATAL_ERROR,
                JSON_ADDR_POLL_SEC,
                setting.addrPollSec.value());
        }
        if (setting.bufferTimeMs.value() < 0)
        {
            setting.enabled = false;
            LOGM_ERROR(
                Config::TAG,
                "*** %s: Config %s value %ld must be non-negative",
                DeviceClient::DC_FATAL_ERROR,
                JSON_BUFFER_TIME_MS,
                setting.bufferTimeMs.value());
        }
        if (setting.bufferSize.value() < 0)
        {
            setting.enabled = false;
            LOGM_ERROR(
                Config::TAG,
                "*** %s: Config %s value %ld must be non-negative",
                DeviceClient::DC_FATAL_ERROR,
                JSON_BUFFER_SIZE,
                setting.bufferSize.value());
        }
        if (setting.heartbeatTimeSec.value() < 0)
        {
            setting.enabled = false;
            LOGM_ERROR(
                Config::TAG,
                "*** %s: Config %s value %ld must be non-negative",
                DeviceClient::DC_FATAL_ERROR,
                JSON_HEARTBEAT_TIME_SEC,
                setting.heartbeatTimeSec.value());
        }

        // Validate the buffer capcity.
        if (setting.bufferCapacity.value() < BUF_CAPACITY_BYTES_MIN)
        {
            setting.enabled = false;
            LOGM_ERROR(
                Config::TAG,
                "*** %s: Config %s value %ld is less than minimum %ld",
                DeviceClient::DC_FATAL_ERROR,
                JSON_BUFFER_CAPACITY,
                setting.bufferCapacity.value(),
                BUF_CAPACITY_BYTES_MIN);
        }

        // If at least one sensor is valid, then enable the feature.
        if (setting.enabled)
        {
            atLeastOneValidSensor = true;
        }
    }

    return atLeastOneValidSensor;
}

void PlainConfig::SensorPublish::SerializeToObject(Crt::JsonObject &object) const
{
    Aws::Crt::Vector<Aws::Crt::JsonObject> sensors;
    for (const auto &entry : settings)
    {
        Aws::Crt::JsonObject sensor;

        if (entry.name.has_value() && entry.name->c_str())
        {
            sensor.WithString(JSON_NAME, entry.name->c_str());
        }

        sensor.WithBool(JSON_ENABLED, entry.enabled);

        if (entry.addr.has_value() && entry.addr->c_str())
        {
            sensor.WithString(JSON_ADDR, entry.addr->c_str());
        }

        if (entry.addrPollSec.has_value())
        {
            sensor.WithInt64(JSON_ADDR_POLL_SEC, entry.addrPollSec.value());
        }

        if (entry.bufferTimeMs.has_value())
        {
            sensor.WithInt64(JSON_BUFFER_TIME_MS, entry.bufferTimeMs.value());
        }

        if (entry.bufferSize.has_value())
        {
            sensor.WithInt64(JSON_BUFFER_SIZE, entry.bufferSize.value());
        }

        if (entry.bufferCapacity.has_value())
        {
            sensor.WithInt64(JSON_BUFFER_CAPACITY, entry.bufferCapacity.value());
        }

        if (entry.eomDelimiter.has_value() && entry.eomDelimiter->c_str())
        {
            sensor.WithString(JSON_EOM_DELIMITER, entry.eomDelimiter->c_str());
        }

        if (entry.mqttTopic.has_value() && entry.mqttTopic->c_str())
        {
            sensor.WithString(JSON_MQTT_TOPIC, entry.mqttTopic->c_str());
        }

        if (entry.mqttDeadLetterTopic.has_value() && entry.mqttDeadLetterTopic->c_str())
        {
            sensor.WithString(JSON_MQTT_DEAD_LETTER_TOPIC, entry.mqttDeadLetterTopic->c_str());
        }

        if (entry.mqttHeartbeatTopic.has_value() && entry.mqttHeartbeatTopic->c_str())
        {
            sensor.WithString(JSON_MQTT_HEARTBEAT_TOPIC, entry.mqttHeartbeatTopic->c_str());
        }

        if (entry.heartbeatTimeSec.has_value())
        {
            sensor.WithInt64(JSON_HEARTBEAT_TIME_SEC, entry.heartbeatTimeSec.value());
        }

        sensors.push_back(sensor);
    }

    object.WithArray(JSON_SENSORS, sensors);
}

constexpr char Config::TAG[];
constexpr char Config::DEFAULT_CONFIG_DIR[];
constexpr char Config::DEFAULT_KEY_DIR[];
constexpr char Config::DEFAULT_CONFIG_FILE[];
constexpr char Config::CLI_HELP[];
constexpr char Config::CLI_VERSION[];
constexpr char Config::CLI_EXPORT_DEFAULT_SETTINGS[];
constexpr char Config::CLI_CONFIG_FILE[];
constexpr char Config::DEFAULT_FLEET_PROVISIONING_RUNTIME_CONFIG_FILE[];
constexpr char Config::DEFAULT_SAMPLE_SHADOW_OUTPUT_DIR[];
constexpr char Config::DEFAULT_SAMPLE_SHADOW_DOCUMENT_FILE[];
constexpr char Config::DEFAULT_HTTP_PROXY_CONFIG_FILE[];

bool Config::CheckTerminalArgs(int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
    {
        std::string currentArg = argv[i];
        if (currentArg == CLI_HELP)
        {
            PrintHelpMessage();
            return true;
        }
        if (currentArg == CLI_VERSION)
        {
            PrintVersion();
            return true;
        }
    }
    return false;
}

bool Config::ParseCliArgs(int argc, char **argv, CliArgs &cliArgs)
{
    struct ArgumentDefinition
    {
        std::string cliFlag;                                              // Cli flag to look for
        bool additionalArg;                                               // Does this take an addition argument?
        std::function<void(const std::string &additionalArg)> extraSteps; // Function to call if this is found
    };

    ArgumentDefinition argumentDefinitions[] = {
        {CLI_EXPORT_DEFAULT_SETTINGS, true, [](const string &additionalArg) { ExportDefaultSetting(additionalArg); }},
        {CLI_CONFIG_FILE, true, nullptr},

        {PlainConfig::CLI_ENDPOINT, true, nullptr},
        {PlainConfig::CLI_CERT, true, nullptr},
        {PlainConfig::CLI_KEY, true, nullptr},
        {PlainConfig::CLI_ROOT_CA, true, nullptr},
        {PlainConfig::CLI_THING_NAME, true, nullptr},

        {PlainConfig::LogConfig::CLI_LOG_LEVEL, true, nullptr},
        {PlainConfig::LogConfig::CLI_LOG_TYPE, true, nullptr},
        {PlainConfig::LogConfig::CLI_LOG_FILE, true, nullptr},
        {PlainConfig::LogConfig::CLI_ENABLE_SDK_LOGGING, false, nullptr},
        {PlainConfig::LogConfig::CLI_SDK_LOG_LEVEL, true, nullptr},
        {PlainConfig::LogConfig::CLI_SDK_LOG_FILE, true, nullptr},

        {PlainConfig::Jobs::CLI_ENABLE_JOBS, true, nullptr},
        {PlainConfig::Jobs::CLI_HANDLER_DIR, true, nullptr},

        {PlainConfig::Tunneling::CLI_ENABLE_TUNNELING, true, nullptr},
        {PlainConfig::Tunneling::CLI_TUNNELING_REGION, true, nullptr},
        {PlainConfig::Tunneling::CLI_TUNNELING_SERVICE, true, nullptr},
        {PlainConfig::Tunneling::CLI_TUNNELING_DISABLE_NOTIFICATION, false, nullptr},

        {PlainConfig::DeviceDefender::CLI_ENABLE_DEVICE_DEFENDER, true, nullptr},
        {PlainConfig::DeviceDefender::CLI_DEVICE_DEFENDER_INTERVAL, true, nullptr},

        {PlainConfig::FleetProvisioning::CLI_ENABLE_FLEET_PROVISIONING, true, nullptr},
        {PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_TEMPLATE_NAME, true, nullptr},
        {PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_TEMPLATE_PARAMETERS, true, nullptr},
        {PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_CSR_FILE, true, nullptr},
        {PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_DEVICE_KEY, true, nullptr},

        {PlainConfig::PubSub::CLI_ENABLE_PUB_SUB, true, nullptr},
        {PlainConfig::PubSub::CLI_PUB_SUB_PUBLISH_TOPIC, true, nullptr},
        {PlainConfig::PubSub::CLI_PUB_SUB_PUBLISH_FILE, true, nullptr},
        {PlainConfig::PubSub::CLI_PUB_SUB_SUBSCRIBE_TOPIC, true, nullptr},
        {PlainConfig::PubSub::CLI_PUB_SUB_SUBSCRIBE_FILE, true, nullptr},

        {PlainConfig::SampleShadow::CLI_ENABLE_SAMPLE_SHADOW, true, nullptr},
        {PlainConfig::SampleShadow::CLI_SAMPLE_SHADOW_NAME, true, nullptr},
        {PlainConfig::SampleShadow::CLI_SAMPLE_SHADOW_INPUT_FILE, true, nullptr},
        {PlainConfig::SampleShadow::CLI_SAMPLE_SHADOW_OUTPUT_FILE, true, nullptr},

        {PlainConfig::ConfigShadow::CLI_ENABLE_CONFIG_SHADOW, true, nullptr},

        {PlainConfig::SecureElement::CLI_ENABLE_SECURE_ELEMENT, true, nullptr},
        {PlainConfig::SecureElement::CLI_PKCS11_LIB, true, nullptr},
        {PlainConfig::SecureElement::CLI_SECURE_ELEMENT_PIN, true, nullptr},
        {PlainConfig::SecureElement::CLI_SECURE_ELEMENT_KEY_LABEL, true, nullptr},
        {PlainConfig::SecureElement::CLI_SECURE_ELEMENT_SLOT_ID, true, nullptr},
        {PlainConfig::SecureElement::CLI_SECURE_ELEMENT_TOKEN_LABEL, true, nullptr},
        {PlainConfig::HttpProxyConfig::CLI_HTTP_PROXY_CONFIG_PATH, true, nullptr}};

    map<string, ArgumentDefinition> argumentDefinitionMap;
    for (const auto &i : argumentDefinitions)
    {
        argumentDefinitionMap[i.cliFlag] = i;
    }
    cliArgs.clear();
    for (int i = 1; i < argc; i++)
    {
        std::string currentArg = argv[i];
        auto search = argumentDefinitionMap.find(currentArg);
        if (search == argumentDefinitionMap.end())
        {
            LOGM_ERROR(
                TAG,
                "*** %s: Unrecognised command line argument: %s ***",
                DeviceClient::DC_FATAL_ERROR,
                Sanitize(currentArg).c_str());
            return false;
        }

        if (cliArgs.find(currentArg) != cliArgs.end())
        {
            LOGM_ERROR(
                TAG,
                "*** %s: Command Line argument '%s' cannot be specified more than once ***",
                DeviceClient::DC_FATAL_ERROR,
                Sanitize(currentArg).c_str());
            return false;
        }

        string additionalArg;
        if (search->second.additionalArg)
        {
            if (i + 1 >= argc)
            {
                LOGM_ERROR(
                    TAG,
                    "*** *s: Command Line argument '%s' was passed without specifying addition argument ***",
                    DeviceClient::DC_FATAL_ERROR,
                    Sanitize(currentArg).c_str());
                return false;
            }

            additionalArg = argv[++i];
        }
        cliArgs[currentArg] = additionalArg; // Saving the argument here

        if (search->second.extraSteps)
        {
            search->second.extraSteps(additionalArg);
        }
    }
    return true;
}

bool Config::init(const CliArgs &cliArgs)
{
#if defined(EXCLUDE_JOBS)
    config.jobs.enabled = false;
#endif

#if defined(EXCLUDE_ST)
    config.tunneling.enabled = false;
#endif

    try
    {
        string filename = Config::DEFAULT_CONFIG_FILE;
        bool bReadConfigFile = FileUtils::FileExists(filename);

        if (cliArgs.count(Config::CLI_CONFIG_FILE))
        {
            filename = cliArgs.at(Config::CLI_CONFIG_FILE);
            if (!FileUtils::FileExists(filename))
            {
                LOGM_ERROR(
                    TAG,
                    "*** %s: Config file specified in the CLI doesn't exist: '%s' ***",
                    DeviceClient::DC_FATAL_ERROR,
                    Sanitize(filename).c_str());
                return false;
            }

            bReadConfigFile = true;
        }

        if (bReadConfigFile && !ParseConfigFile(filename, DEVICE_CLIENT_ESSENTIAL_CONFIG))
        {
            LOGM_ERROR(
                TAG,
                "*** %s: Unable to Parse Config file: '%s' ***",
                DeviceClient::DC_FATAL_ERROR,
                Sanitize(filename).c_str());
            return false;
        }

        if (!config.LoadFromCliArgs(cliArgs))
        {
            return false;
        }

        if (!config.LoadFromEnvironment())
        {
            return false;
        }

#if !defined(DISABLE_MQTT)
        // ST_COMPONENT_MODE does not require any settings besides those for Secure Tunneling
        if (ParseConfigFile(
                Config::DEFAULT_FLEET_PROVISIONING_RUNTIME_CONFIG_FILE, FLEET_PROVISIONING_RUNTIME_CONFIG) &&
            ValidateAndStoreRuntimeConfig())
        {
            LOGM_INFO(
                TAG,
                "Successfully fetched Runtime config file '%s' and validated its content.",
                Config::DEFAULT_FLEET_PROVISIONING_RUNTIME_CONFIG_FILE);
        }
#endif

        // ST_COMPONENT_MODE does not require any settings besides those for Secure Tunneling
        if (ParseConfigFile(config.httpProxyConfig.proxyConfigPath->c_str(), HTTP_PROXY_CONFIG) &&
            config.httpProxyConfig.httpProxyEnabled)
        {
            if (!ValidateAndStoreHttpProxyConfig())
            {
                LOGM_ERROR(
                    TAG,
                    "*** %s: Unable to Parse HTTP proxy Config file: '%s' ***",
                    DeviceClient::DC_FATAL_ERROR,
                    Sanitize(config.httpProxyConfig.proxyConfigPath->c_str()).c_str());
                return false;
            }
            LOGM_INFO(
                TAG,
                "Successfully fetched http proxy config file '%s' and validated its content.",
                config.httpProxyConfig.proxyConfigPath->c_str());
            return true;
        }

        return config.Validate();
    }
    catch (const std::exception &e)
    {
        LOGM_ERROR(TAG, "Error while initializing configuration: %s", e.what());
        return false;
    }
}

bool Config::ValidateAndStoreRuntimeConfig()
{
    // check if all values are present and also check if the files are present then only overwrite values

    if (!config.fleetProvisioningRuntimeConfig.Validate())
    {
        LOGM_ERROR(
            TAG,
            "Failed to Validate runtime configurations. Please check '%s' file",
            Config::DEFAULT_FLEET_PROVISIONING_RUNTIME_CONFIG_FILE);
        return false;
    }
    config.cert = config.fleetProvisioningRuntimeConfig.cert.value().c_str();
    config.key = config.fleetProvisioningRuntimeConfig.key.value().c_str();
    config.thingName = config.fleetProvisioningRuntimeConfig.thingName.value().c_str();
    return true;
}

bool Config::ValidateAndStoreHttpProxyConfig() const
{
    // check if all values are present and also check if the files are present then only overwrite values
    if (!config.httpProxyConfig.Validate())
    {
        LOGM_ERROR(
            TAG,
            "Failed to Validate http proxy configurations. Please check '%s' file",
            Config::DEFAULT_HTTP_PROXY_CONFIG_FILE);
        return false;
    }

    return true;
}

bool Config::ParseConfigFile(const string &file, ConfigFileType configFileType)
{
    string expandedPath = FileUtils::ExtractExpandedPath(file.c_str());
    if (!FileUtils::FileExists(expandedPath))
    {
        switch (configFileType)
        {
            case DEVICE_CLIENT_ESSENTIAL_CONFIG:
            {
                LOGM_DEBUG(TAG, "Unable to open config file %s, file does not exist", Sanitize(expandedPath).c_str());
                break;
            }
            case FLEET_PROVISIONING_RUNTIME_CONFIG:
            {
                LOG_DEBUG(
                    TAG,
                    "Did not find a runtime configuration file, assuming Fleet Provisioning has not run for this "
                    "device");
                break;
            }
            case HTTP_PROXY_CONFIG:
            {
                LOGM_DEBUG(
                    TAG,
                    "Did not find a http proxy config file %s, assuming HTTP proxy is disabled on this device",
                    Sanitize(expandedPath).c_str());
                break;
            }
            default:
            {
                LOG_ERROR(TAG, "Unhandled config file type when trying to load from disk: file does not exist");
            }
        };

        return false;
    }

    size_t incomingFileSize = FileUtils::GetFileSize(file);
    if (incomingFileSize > Config::MAX_CONFIG_SIZE)
    {
        LOGM_WARN(
            TAG,
            "Refusing to open config file %s, file size %zu bytes is greater than allowable limit of %zu bytes",
            Sanitize(file).c_str(),
            incomingFileSize,
            Config::MAX_CONFIG_SIZE);
        return false;
    }

    string configFileParentDir = FileUtils::ExtractParentDirectory(expandedPath.c_str());
    FileUtils::ValidateFilePermissions(configFileParentDir, Permissions::CONFIG_DIR, false);
    switch (configFileType)
    {
        case DEVICE_CLIENT_ESSENTIAL_CONFIG:
        {
            FileUtils::ValidateFilePermissions(expandedPath.c_str(), Permissions::CONFIG_FILE, false);
            break;
        }
        case FLEET_PROVISIONING_RUNTIME_CONFIG:
        {
            FileUtils::ValidateFilePermissions(expandedPath.c_str(), Permissions::RUNTIME_CONFIG_FILE, false);
            break;
        }
        case HTTP_PROXY_CONFIG:
        {
            FileUtils::ValidateFilePermissions(expandedPath.c_str(), Permissions::HTTP_PROXY_CONFIG_FILE, false);
            break;
        }
        default:
        {
            LOGM_ERROR(
                TAG,
                "Undefined config type of file %s, file permission was not able to be verified",
                expandedPath.c_str());
        }
    }

    ifstream setting(expandedPath.c_str());
    if (!setting.is_open())
    {
        LOGM_ERROR(TAG, "Unable to open file: '%s'", Sanitize(expandedPath).c_str());
        return false;
    }

    std::string contents((std::istreambuf_iterator<char>(setting)), std::istreambuf_iterator<char>());
    auto jsonObj = Aws::Crt::JsonObject(contents.c_str());
    if (!jsonObj.WasParseSuccessful())
    {
        LOGM_ERROR(
            TAG, "Couldn't parse JSON config file. GetErrorMessage returns: %s", jsonObj.GetErrorMessage().c_str());
        return false;
    }
    auto jsonView = Aws::Crt::JsonView(jsonObj);
    switch (configFileType)
    {
        case DEVICE_CLIENT_ESSENTIAL_CONFIG:
        {
            config.LoadFromJson(jsonView);
            break;
        }
        case FLEET_PROVISIONING_RUNTIME_CONFIG:
        {
            config.LoadFromJson(jsonView);
            break;
        }
        case HTTP_PROXY_CONFIG:
        {
            config.httpProxyConfig.LoadFromJson(jsonView);
            break;
        }
        default:
        {
            LOGM_ERROR(
                TAG,
                "Undefined config type of file %s, was not able to parse config into json object",
                expandedPath.c_str());
        }
    }

#if !defined(DISABLE_MQTT)
    LOGM_INFO(TAG, "Successfully fetched JSON config file: %s", Sanitize(contents).c_str());
#endif
    setting.close();

    return true;
}

void Config::PrintHelpMessage()
{
    const char *helpMessageTemplate =
        "\n\n\tAWS IoT Device Client BINARY\n"
        "\n"
        "For more documentation, see https://github.com/awslabs/aws-iot-device-client\n"
        "\n"
        "Available sub-commands:\n"
        "\n"
        "%s:\t\t\t\t\t\t\t\t\tGet more help on commands\n"
        "%s:\t\t\t\t\t\t\t\tOutput current version\n"
        "%s <JSON-File-Location>:\t\t\t\tExport default settings for the AWS IoT Device Client binary to the specified "
        "file "
        "and exit "
        "program\n"
        "%s <JSON-File-Location>:\t\t\t\t\tTake settings defined in the specified JSON file and start the binary\n"
        "%s <[DEBUG, INFO, WARN, ERROR]>:\t\t\t\tSpecify the log level for the AWS IoT Device Client\n"
        "%s <[STDOUT, FILE]>:\t\t\t\t\t\tSpecify the logger implementation to use.\n"
        "%s <File-Location>:\t\t\t\t\t\tWrite logs to specified log file when using the file logger.\n"
        "%s \t\t\t\t\t\t\tEnable SDK Logging.\n"
        "%s <[Trace, Debug, Info, Warn, Error, Fatal]>:\t\tSpecify the log level for the SDK\n"
        "%s <File-Location>:\t\t\t\t\t\tWrite SDK logs to specified log file.\n"
        "%s [true|false]:\t\t\t\t\t\tEnables/Disables Jobs feature\n"
        "%s [true|false]:\t\t\t\t\tEnables/Disables Tunneling feature\n"
        "%s [true|false]:\t\t\t\t\tEnables/Disables Device Defender feature\n"
        "%s [true|false]:\t\t\t\tEnables/Disables Fleet Provisioning feature\n"
        "%s [true|false]:\t\t\t\t\t\tEnables/Disables Pub/Sub Sample feature\n"
        "%s [true|false]:\t\t\t\t\tEnables/Disables Sample Shadow feature\n"
        "%s [true|false]:\t\t\t\t\tEnables/Disables Config Shadow feature\n"
        "%s [true|false]:\t\t\t\t\t\tEnables/Disables Secure Element Configuration\n"
        "%s <endpoint-value>:\t\t\t\t\t\tUse Specified Endpoint\n"
        "%s <Cert-Location>:\t\t\t\t\t\t\tUse Specified Cert file\n"
        "%s <Key-Location>:\t\t\t\t\t\t\tUse Specified Key file\n"
        "%s <Root-CA-Location>:\t\t\t\t\t\tUse Specified Root-CA file\n"
        "%s <thing-name-value/client-id-value>:\t\t\tUse Specified Thing Name (Also used as Client ID)\n"
        "%s <Jobs-handler-directory>:\t\t\t\tUse specified directory to find job handlers\n"
        "%s <region>:\t\t\t\t\t\tUse Specified AWS Region for Secure Tunneling\n"
        "%s <service>:\t\t\t\t\t\tConnect secure tunnel to specific service\n"
        "%s:\t\t\t\t\tDisable MQTT new tunnel notification for Secure Tunneling\n"
        "%s <interval>:\t\t\t\t\tPositive integer to publish Device Defender metrics\n"
        "%s <template-name>:\t\t\tUse specified Fleet Provisioning template name\n"
        "%s <template-parameters>:\t\tUse specified Fleet Provisioning template parameters. A JSON object specified as "
        "an escaped string\n"
        "%s <csr-file-path>:\t\t\t\t\t\tUse specified CSR file to generate a certificate by keeping user private key "
        "secure. If the CSR file is specified without also specifying a device private key, the Device Client will use "
        "Claim Certificate and Private key to generate new Certificate and Private Key while provisioning the device\n"
        "%s <device-key-path>:\t\t\t\t\t\tUse specified device key to connect to IoT core after provisioning using csr "
        "file is completed. If the CSR file is specified without also specifying a device private key, the Device "
        "Client will use Claim Certificate and Private key to generate new Certificate and Private Key while "
        "provisioning the device\n"
        "%s <publish-topic>:\t\t\t\t\tThe topic the Pub/Sub sample feature will publish to\n"
        "%s <path/to/publish/file>:\t\t\t\t\tThe file the Pub/Sub sample feature will read from when publishing\n"
        "%s <subscribe-topic>:\t\t\t\t\tThe topic the Pub/Sub sample feature will receive messages on\n"
        "%s <path/to/sub/file>:\t\t\t\t\tThe file the Pub/Sub sample feature will write received messaged to\n"
        "%s <shadow-name>:\t\t\t\t\t\tThe name of shadow SampleShadow feature will create or update\n"
        "%s <shadow-input-file>:\t\t\t\tThe file the Sample Shadow feature will read from when updating shadow data\n"
        "%s <shadow-output-file>:\t\t\t\tThe file the Sample Shadow feature will write the latest shadow document "
        "to\n"
        "%s <pkcs11-lib-path>:\t\t\t\t\tThe file path to PKCS#11 library\n"
        "%s <secure-element-pin>:\t\t\t\t\tThe user PIN for logging into PKCS#11 token.\n"
        "%s <secure-element-key-label>:\t\t\t\t\tThe Label of private key on the PKCS#11 token (optional). \n"
        "%s <secure-element-slot-id>:\t\t\t\t\tThe Slot ID containing PKCS#11 token to use (optional).\n"
        "%s <secure-element-token-label>:\t\t\t\t\tThe Label of the PKCS#11 token to use (optional).\n"
        "%s <http-proxy-config-file>:\t\t\t\tUse specified file path to load HTTP proxy configs\n";

    cout << FormatMessage(
        helpMessageTemplate,
        CLI_HELP,
        CLI_VERSION,
        CLI_EXPORT_DEFAULT_SETTINGS,
        CLI_CONFIG_FILE,
        PlainConfig::LogConfig::CLI_LOG_LEVEL,
        PlainConfig::LogConfig::CLI_LOG_TYPE,
        PlainConfig::LogConfig::CLI_LOG_FILE,
        PlainConfig::LogConfig::CLI_ENABLE_SDK_LOGGING,
        PlainConfig::LogConfig::CLI_SDK_LOG_LEVEL,
        PlainConfig::LogConfig::CLI_SDK_LOG_FILE,
        PlainConfig::Jobs::CLI_ENABLE_JOBS,
        PlainConfig::Tunneling::CLI_ENABLE_TUNNELING,
        PlainConfig::DeviceDefender::CLI_ENABLE_DEVICE_DEFENDER,
        PlainConfig::FleetProvisioning::CLI_ENABLE_FLEET_PROVISIONING,
        PlainConfig::PubSub::CLI_ENABLE_PUB_SUB,
        PlainConfig::SampleShadow::CLI_ENABLE_SAMPLE_SHADOW,
        PlainConfig::ConfigShadow::CLI_ENABLE_CONFIG_SHADOW,
        PlainConfig::SecureElement::CLI_ENABLE_SECURE_ELEMENT,
        PlainConfig::CLI_ENDPOINT,
        PlainConfig::CLI_CERT,
        PlainConfig::CLI_KEY,
        PlainConfig::CLI_ROOT_CA,
        PlainConfig::CLI_THING_NAME,
        PlainConfig::Jobs::CLI_HANDLER_DIR,
        PlainConfig::Tunneling::CLI_TUNNELING_REGION,
        PlainConfig::Tunneling::CLI_TUNNELING_SERVICE,
        PlainConfig::Tunneling::CLI_TUNNELING_DISABLE_NOTIFICATION,
        PlainConfig::DeviceDefender::CLI_DEVICE_DEFENDER_INTERVAL,
        PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_TEMPLATE_NAME,
        PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_TEMPLATE_PARAMETERS,
        PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_CSR_FILE,
        PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_DEVICE_KEY,
        PlainConfig::PubSub::CLI_PUB_SUB_PUBLISH_TOPIC,
        PlainConfig::PubSub::CLI_PUB_SUB_PUBLISH_FILE,
        PlainConfig::PubSub::CLI_PUB_SUB_SUBSCRIBE_TOPIC,
        PlainConfig::PubSub::CLI_PUB_SUB_SUBSCRIBE_FILE,
        PlainConfig::SampleShadow::CLI_SAMPLE_SHADOW_NAME,
        PlainConfig::SampleShadow::CLI_SAMPLE_SHADOW_INPUT_FILE,
        PlainConfig::SampleShadow::CLI_SAMPLE_SHADOW_OUTPUT_FILE,
        PlainConfig::SecureElement::CLI_PKCS11_LIB,
        PlainConfig::SecureElement::CLI_SECURE_ELEMENT_PIN,
        PlainConfig::SecureElement::CLI_SECURE_ELEMENT_KEY_LABEL,
        PlainConfig::SecureElement::CLI_SECURE_ELEMENT_SLOT_ID,
        PlainConfig::SecureElement::CLI_SECURE_ELEMENT_TOKEN_LABEL,
        PlainConfig::HttpProxyConfig::CLI_HTTP_PROXY_CONFIG_PATH);
}

void Config::PrintVersion()
{
    cout << DEVICE_CLIENT_VERSION_FULL << endl;
}

bool Config::ExportDefaultSetting(const string &file)
{
    string jsonTemplate = R"({
    "%s": "<replace_with_endpoint_value>",
    "%s": "<replace_with_certificate_file_path>",
    "%s": "<replace_with_private_key_file_path>",
    "%s": "<replace_with_root_ca_file_path>",
    "%s": "<replace_with_thing_name>",
    "%s": {
        "%s": "DEBUG",
        "%s": "FILE",
        "%s": "%s",
        "%s": false,
        "%s": "TRACE",
        "%s": "%s"
    },
    "%s": {
        "%s": true,
        "%s": "<replace_with_job_handler_directory_path>"
    },
    "%s": {
        "%s": true
    },
    "%s": {
        "%s": true,
        "%s": <replace_with_interval>
    },
    "%s": {
        "%s": false,
        "%s": "<replace_with_template_name>",
        "%s": "<replace_with_template_parameters>",
        "%s": "<replace_with_csr_file_path>",
        "%s": "<replace_with_device_private_key_file_path>"
    },
    "%s": {
        "%s": {
            "%s": false,
            "%s": "<replace_with_publish_topic>",
            "%s": "<replace_with_publish_file_path>",
            "%s": "<replace_with_subscribe_topic>",
            "%s": "<replace_with_subscribe_file_path>"
        }
    },
    "%s": {
		"%s": false,
		"%s": "<replace_with_shadow_name>",
		"%s": "<replace_with_shaodw_input_file_path>",
		"%s": "<replace_with_shaodw_output_file_path>"
	},
    "%s": {
        "%s": false
    },
    "%s": {
		"%s": false,
		"%s": "<replace_with_pkcs11_lib_path>",
		"%s": "<replace_with_secure_element_pin>",
		"%s": "<replace_with_secure_element_key_label>",
		"%s": replace_with_secure_element_slot_id_integer,
		"%s": "<replace_with_secure_element_token_label>"
	},
        "%s": [
            "%s": false,
            "%s": "<replace>",
            "%s": "<replace>",
            "%s": replace,
            "%s": replace,
            "%s": replace,
            "%s": replace,
            "%s": "<replace>",
            "%s": "<replace>",
            "%s": "<replace>",
            "%s": "<replace>",
            "%s": replace
        ]
    }
}
)";

    ofstream clientConfig(file);
    if (!clientConfig.is_open())
    {
        LOGM_ERROR(TAG, "Unable to open file: '%s'", Sanitize(file).c_str());
        return false;
    }
    clientConfig << FormatMessage(
        jsonTemplate.c_str(),
        PlainConfig::JSON_KEY_ENDPOINT,
        PlainConfig::JSON_KEY_CERT,
        PlainConfig::JSON_KEY_KEY,
        PlainConfig::JSON_KEY_ROOT_CA,
        PlainConfig::JSON_KEY_THING_NAME,
        PlainConfig::JSON_KEY_LOGGING,
        PlainConfig::LogConfig::JSON_KEY_LOG_LEVEL,
        PlainConfig::LogConfig::JSON_KEY_LOG_TYPE,
        PlainConfig::LogConfig::JSON_KEY_LOG_FILE,
        FileLogger::DEFAULT_LOG_FILE,
        PlainConfig::LogConfig::JSON_KEY_ENABLE_SDK_LOGGING,
        PlainConfig::LogConfig::JSON_KEY_SDK_LOG_LEVEL,
        PlainConfig::LogConfig::JSON_KEY_SDK_LOG_FILE,
        SharedCrtResourceManager::DEFAULT_SDK_LOG_FILE,
        PlainConfig::JSON_KEY_JOBS,
        PlainConfig::Jobs::JSON_KEY_ENABLED,
        PlainConfig::Jobs::JSON_KEY_HANDLER_DIR,
        PlainConfig::JSON_KEY_TUNNELING,
        PlainConfig::Tunneling::JSON_KEY_ENABLED,
        PlainConfig::JSON_KEY_DEVICE_DEFENDER,
        PlainConfig::DeviceDefender::JSON_KEY_ENABLED,
        PlainConfig::DeviceDefender::JSON_KEY_INTERVAL,
        PlainConfig::JSON_KEY_FLEET_PROVISIONING,
        PlainConfig::FleetProvisioning::JSON_KEY_ENABLED,
        PlainConfig::FleetProvisioning::JSON_KEY_TEMPLATE_NAME,
        PlainConfig::FleetProvisioning::JSON_KEY_TEMPLATE_PARAMETERS,
        PlainConfig::FleetProvisioning::JSON_KEY_CSR_FILE,
        PlainConfig::FleetProvisioning::JSON_KEY_DEVICE_KEY,
        PlainConfig::JSON_KEY_SAMPLES,
        PlainConfig::JSON_KEY_PUB_SUB,
        PlainConfig::PubSub::JSON_ENABLE_PUB_SUB,
        PlainConfig::PubSub::JSON_PUB_SUB_PUBLISH_TOPIC,
        PlainConfig::PubSub::JSON_PUB_SUB_PUBLISH_FILE,
        PlainConfig::PubSub::JSON_PUB_SUB_SUBSCRIBE_TOPIC,
        PlainConfig::PubSub::JSON_PUB_SUB_SUBSCRIBE_FILE,
        PlainConfig::JSON_KEY_SAMPLE_SHADOW,
        PlainConfig::SampleShadow::JSON_ENABLE_SAMPLE_SHADOW,
        PlainConfig::SampleShadow::JSON_SAMPLE_SHADOW_NAME,
        PlainConfig::SampleShadow::JSON_SAMPLE_SHADOW_INPUT_FILE,
        PlainConfig::SampleShadow::JSON_SAMPLE_SHADOW_OUTPUT_FILE,
        PlainConfig::JSON_KEY_CONFIG_SHADOW,
        PlainConfig::ConfigShadow::JSON_ENABLE_CONFIG_SHADOW,
        PlainConfig::JSON_KEY_SECURE_ELEMENT,
        PlainConfig::SecureElement::JSON_ENABLE_SECURE_ELEMENT,
        PlainConfig::SecureElement::JSON_PKCS11_LIB,
        PlainConfig::SecureElement::JSON_SECURE_ELEMENT_PIN,
        PlainConfig::SecureElement::JSON_SECURE_ELEMENT_KEY_LABEL,
        PlainConfig::SecureElement::JSON_SECURE_ELEMENT_SLOT_ID,
        PlainConfig::SecureElement::JSON_SECURE_ELEMENT_TOKEN_LABEL,
        PlainConfig::PlainConfig::JSON_KEY_SENSOR_PUBLISH,
        PlainConfig::SensorPublish::JSON_SENSORS,
        PlainConfig::SensorPublish::JSON_ENABLED,
        PlainConfig::SensorPublish::JSON_NAME,
        PlainConfig::SensorPublish::JSON_ADDR,
        PlainConfig::SensorPublish::JSON_ADDR_POLL_SEC,
        PlainConfig::SensorPublish::JSON_BUFFER_TIME_MS,
        PlainConfig::SensorPublish::JSON_BUFFER_SIZE,
        PlainConfig::SensorPublish::JSON_BUFFER_CAPACITY,
        PlainConfig::SensorPublish::JSON_EOM_DELIMITER,
        PlainConfig::SensorPublish::JSON_MQTT_TOPIC,
        PlainConfig::SensorPublish::JSON_MQTT_DEAD_LETTER_TOPIC,
        PlainConfig::SensorPublish::JSON_MQTT_HEARTBEAT_TOPIC,
        PlainConfig::SensorPublish::JSON_HEARTBEAT_TIME_SEC);

    clientConfig.close();
    LOGM_INFO(TAG, "Exported settings to: %s", Sanitize(file).c_str());

    chmod(file.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    FileUtils::ValidateFilePermissions(file.c_str(), Permissions::CONFIG_FILE, false);
    return true;
}

string Config::ExpandDefaultConfigDir(bool removeTrailingSeparator)
{
    string expandedConfigDir = FileUtils::ExtractExpandedPath(DEFAULT_CONFIG_DIR);
    if (removeTrailingSeparator)
    {
        return Util::TrimRightCopy(expandedConfigDir, string{Config::PATH_DIRECTORY_SEPARATOR});
    }
    return expandedConfigDir;
}