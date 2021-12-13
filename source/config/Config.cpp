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

#include "../util/FileUtils.h"
#include "../util/StringUtils.h"

#include <algorithm>
#include <aws/crt/JsonObject.h>
#include <iostream>
#include <map>
#include <stdexcept>
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

constexpr int Permissions::KEY_DIR;
constexpr int Permissions::ROOT_CA_DIR;
constexpr int Permissions::CERT_DIR;
constexpr int Permissions::CONFIG_DIR;
constexpr int Permissions::LOG_DIR;
constexpr int Permissions::PRIVATE_KEY;
constexpr int Permissions::PUBLIC_CERT;
constexpr int Permissions::LOG_FILE;
constexpr int Permissions::CONFIG_FILE;
constexpr int Permissions::RUNTIME_CONFIG_FILE;
constexpr int Permissions::JOB_HANDLER;
constexpr int Permissions::PUB_SUB_FILES;
constexpr int Permissions::SAMPLE_SHADOW_FILES;

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
            rootCa = FileUtils::ExtractExpandedPath(json.GetString(jsonKey).c_str());
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
        if (json.ValueExists(jsonKey))
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
        rootCa = FileUtils::ExtractExpandedPath(cliArgs.at(PlainConfig::CLI_ROOT_CA).c_str());
    }
    if (cliArgs.count(PlainConfig::CLI_THING_NAME))
    {
        thingName = cliArgs.at(PlainConfig::CLI_THING_NAME).c_str();
    }

    return logConfig.LoadFromCliArgs(cliArgs) && jobs.LoadFromCliArgs(cliArgs) && tunneling.LoadFromCliArgs(cliArgs) &&
           deviceDefender.LoadFromCliArgs(cliArgs) && fleetProvisioning.LoadFromCliArgs(cliArgs) &&
           pubSub.LoadFromCliArgs(cliArgs) && sampleShadow.LoadFromCliArgs(cliArgs) &&
           configShadow.LoadFromCliArgs(cliArgs);
}

bool PlainConfig::LoadFromEnvironment()
{
    return logConfig.LoadFromEnvironment() && jobs.LoadFromEnvironment() && tunneling.LoadFromEnvironment() &&
           deviceDefender.LoadFromEnvironment() && fleetProvisioning.LoadFromEnvironment() &&
           fleetProvisioningRuntimeConfig.LoadFromEnvironment() && pubSub.LoadFromEnvironment() &&
           sampleShadow.LoadFromEnvironment() && configShadow.LoadFromEnvironment();
}

bool PlainConfig::Validate() const
{
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
    if (!key.has_value() || key->empty())
    {
        LOGM_ERROR(Config::TAG, "*** %s: Private Key is missing ***", DeviceClient::DC_FATAL_ERROR);
        return false;
    }
    else if (!FileUtils::IsValidFilePath(key->c_str()))
    {
        return false;
    }
    if (!rootCa.has_value() || rootCa->empty())
    {
        LOGM_ERROR(Config::TAG, "*** %s: Root CA is missing ***", DeviceClient::DC_FATAL_ERROR);
        return false;
    }
    else if (!FileUtils::IsValidFilePath(rootCa->c_str()))
    {
        return false;
    }
    if (!thingName.has_value() || thingName->empty())
    {
        LOGM_ERROR(Config::TAG, "*** %s: Thing name is missing ***", DeviceClient::DC_FATAL_ERROR);
        return false;
    }
#endif
    if (!logConfig.Validate())
    {
        return false;
    }
#if !defined(EXCLUDE_JOBS)
    if (!jobs.Validate())
    {
        return false;
    }
#endif
#if !defined(EXCLUDE_DD)
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
#if !defined(EXCLUDE_FP)
    if (!fleetProvisioning.Validate())
    {
        return false;
    }
#endif
#if !defined(EXCLUDE_PUBSUB)
    if (!pubSub.Validate())
    {
        return false;
    }
#endif
#if !defined(EXCLUDE_SHADOW)
    if (!sampleShadow.Validate() || !configShadow.Validate())
    {
        return false;
    }
#endif

    return true;
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

int PlainConfig::LogConfig::ParseDeviceClientLogLevel(string level)
{
    string temp = level;
    std::transform(temp.begin(), temp.end(), temp.begin(), [](unsigned char c) { return std::toupper(c); });

    if ("DEBUG" == temp)
    {
        return (int)LogLevel::DEBUG;
    }
    else if ("INFO" == temp)
    {
        return (int)LogLevel::INFO;
    }
    else if ("WARN" == temp)
    {
        return (int)LogLevel::WARN;
    }
    else if ("ERROR" == temp)
    {
        return (int)LogLevel::ERROR;
    }
    else
    {
        throw std::invalid_argument(FormatMessage(
            "Provided log level %s is not a known log level for the AWS IoT Device Client", Sanitize(level).c_str()));
    }
}

Aws::Crt::LogLevel PlainConfig::LogConfig::ParseSDKLogLevel(string level)
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

string PlainConfig::LogConfig::ParseDeviceClientLogType(string value)
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
    if (json.ValueExists(jsonKey))
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
    if (json.ValueExists(jsonKey) && json.GetBool(jsonKey))
    {
        if (json.GetBool(jsonKey))
        {
            sdkLoggingEnabled = true;
        }
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
    (void)object;

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
        auto service = cliArgs.at(PlainConfig::Tunneling::CLI_TUNNELING_SERVICE);
#if !defined(EXCLUDE_ST)
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
    (void)object;

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
    (void)object;
    object.WithBool(JSON_KEY_ENABLED, enabled);
    object.WithInteger(JSON_KEY_INTERVAL, interval);
}

constexpr char PlainConfig::FleetProvisioning::CLI_ENABLE_FLEET_PROVISIONING[];
constexpr char PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_TEMPLATE_NAME[];
constexpr char PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_TEMPLATE_PARAMETERS[];
constexpr char PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_CSR_FILE[];
constexpr char PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_DEVICE_KEY[];

constexpr char PlainConfig::FleetProvisioning::JSON_KEY_ENABLED[];
constexpr char PlainConfig::FleetProvisioning::JSON_KEY_TEMPLATE_NAME[];
constexpr char PlainConfig::FleetProvisioning::JSON_KEY_TEMPLATE_PARAMETERS[];
constexpr char PlainConfig::FleetProvisioning::JSON_KEY_CSR_FILE[];
constexpr char PlainConfig::FleetProvisioning::JSON_KEY_DEVICE_KEY[];

bool PlainConfig::FleetProvisioning::LoadFromJson(const Crt::JsonView &json)
{
    const char *jsonKey = JSON_KEY_ENABLED;
    if (json.ValueExists(jsonKey))
    {
        enabled = json.GetBool(jsonKey);
    }

    jsonKey = JSON_KEY_TEMPLATE_NAME;
    if (json.ValueExists(jsonKey))
    {
        if (!json.GetString(jsonKey).empty())
        {
            templateName = json.GetString(jsonKey).c_str();
        }
        else
        {
            LOGM_WARN(Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
        }
    }

    jsonKey = JSON_KEY_TEMPLATE_PARAMETERS;
    if (json.ValueExists(jsonKey))
    {
        if (!json.GetString(jsonKey).empty())
        {
            templateParameters = json.GetString(jsonKey).c_str();
        }
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
            LOGM_WARN(Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
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
            LOGM_WARN(Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
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

    if (csrFile.has_value() && !csrFile->empty())
    {
        if (!FileUtils::IsValidFilePath(csrFile->c_str()))
        {
            return false;
        }
    }

    if (deviceKey.has_value() && !deviceKey->empty())
    {
        if (!FileUtils::IsValidFilePath(deviceKey->c_str()))
        {
            return false;
        }
    }

    return true;
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

    jsonKey = JSON_KEY_THING_NAME;
    if (json.ValueExists(jsonKey))
    {
        thingName = json.GetString(jsonKey).c_str();
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

bool PlainConfig::PubSub::LoadFromJson(const Crt::JsonView &json)
{
    const char *jsonKey = JSON_ENABLE_PUB_SUB;
    if (json.ValueExists(jsonKey))
    {
        enabled = json.GetBool(jsonKey);
    }

    jsonKey = JSON_PUB_SUB_PUBLISH_TOPIC;
    if (json.ValueExists(jsonKey))
    {
        if (!json.GetString(jsonKey).empty())
        {
            publishTopic = json.GetString(jsonKey).c_str();
        }
        else
        {
            LOGM_WARN(Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
        }
    }

    jsonKey = JSON_PUB_SUB_PUBLISH_FILE;
    if (json.ValueExists(jsonKey))
    {
        if (!json.GetString(jsonKey).empty())
        {
            publishFile = FileUtils::ExtractExpandedPath(json.GetString(jsonKey).c_str());
        }
        else
        {
            LOGM_WARN(Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
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
            LOGM_WARN(Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
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
            LOGM_WARN(Config::TAG, "Key {%s} was provided in the JSON configuration file with an empty value", jsonKey);
        }
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
    if (publishFile.has_value() && !publishFile->empty())
    {
        if (FileUtils::IsValidFilePath(publishFile->c_str()))
        {
            if (!FileUtils::ValidateFilePermissions(publishFile.value(), Permissions::PUB_SUB_FILES, true))
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }
    if (!subscribeTopic.has_value() || subscribeTopic->empty())
    {
        LOGM_ERROR(
            Config::TAG,
            "*** %s: Subscribe Topic field must be specified if Pub-Sub sample feature is enabled ***",
            DeviceClient::DC_FATAL_ERROR);
        return false;
    }
    if (subscribeFile.has_value() && !subscribeFile->empty())
    {
        if (FileUtils::IsValidFilePath(subscribeFile->c_str()))
        {
            if (!FileUtils::ValidateFilePermissions(subscribeFile.value(), Permissions::PUB_SUB_FILES, true))
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }
    return true;
}

void PlainConfig::PubSub::SerializeToObject(Crt::JsonObject &object) const
{
    (void)object;
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

bool PlainConfig::SampleShadow::LoadFromJson(const Crt::JsonView &json)
{
    const char *jsonKey = JSON_ENABLE_SAMPLE_SHADOW;
    if (json.ValueExists(jsonKey))
    {
        enabled = json.GetBool(jsonKey);
    }

    jsonKey = JSON_SAMPLE_SHADOW_NAME;
    if (!json.GetString(jsonKey).empty())
    {
        shadowName = json.GetString(jsonKey).c_str();
    }
    else
    {
        LOGM_WARN(
            Config::TAG, "Shadow Name {%s} was provided in the JSON configuration file with an empty value", jsonKey);
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
    if (json.ValueExists(jsonKey))
    {
        if (!json.GetString(jsonKey).empty())
        {
            shadowOutputFile = FileUtils::ExtractExpandedPath(json.GetString(jsonKey).c_str());
        }
        else
        {
            LOGM_WARN(
                Config::TAG,
                "Output file {%s} was provided in the JSON configuration file with an empty value",
                jsonKey);
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
            return false;
        }
    }

    return true;
}

void PlainConfig::SampleShadow::SerializeToObject(Crt::JsonObject &object) const
{
    (void)object;

    object.WithBool(JSON_ENABLE_SAMPLE_SHADOW, enabled);

    if (shadowName->c_str())
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

constexpr char Config::TAG[];
constexpr char Config::DEFAULT_CONFIG_DIR[];
constexpr char Config::DEFAULT_KEY_DIR[];
constexpr char Config::DEFAULT_CONFIG_FILE[];
constexpr char Config::CLI_HELP[];
constexpr char Config::CLI_EXPORT_DEFAULT_SETTINGS[];
constexpr char Config::CLI_CONFIG_FILE[];
constexpr char Config::DEFAULT_FLEET_PROVISIONING_RUNTIME_CONFIG_FILE[];
constexpr char Config::DEFAULT_SAMPLE_SHADOW_OUTPUT_DIR[];

bool Config::ParseCliArgs(int argc, char **argv, CliArgs &cliArgs)
{
    struct ArgumentDefinition
    {
        string cliFlag;     // Cli flag to look for
        bool additionalArg; // Does this take an addition argument?
        bool stopIfFound;   // Should we stop processing more arguments if this is found?
        std::function<void(const string &additionalArg)> extraSteps; // Function to call if this is found
    };
    ArgumentDefinition argumentDefinitions[] = {
        {CLI_HELP, false, true, [](const string &additionalArg) { PrintHelpMessage(); }},
        {CLI_EXPORT_DEFAULT_SETTINGS,
         true,
         true,
         [](const string &additionalArg) { ExportDefaultSetting(additionalArg); }},
        {CLI_CONFIG_FILE, true, false, nullptr},

        {PlainConfig::CLI_ENDPOINT, true, false, nullptr},
        {PlainConfig::CLI_CERT, true, false, nullptr},
        {PlainConfig::CLI_KEY, true, false, nullptr},
        {PlainConfig::CLI_ROOT_CA, true, false, nullptr},
        {PlainConfig::CLI_THING_NAME, true, false, nullptr},

        {PlainConfig::LogConfig::CLI_LOG_LEVEL, true, false, nullptr},
        {PlainConfig::LogConfig::CLI_LOG_TYPE, true, false, nullptr},
        {PlainConfig::LogConfig::CLI_LOG_FILE, true, false, nullptr},
        {PlainConfig::LogConfig::CLI_ENABLE_SDK_LOGGING, false, false, nullptr},
        {PlainConfig::LogConfig::CLI_SDK_LOG_LEVEL, true, false, nullptr},
        {PlainConfig::LogConfig::CLI_SDK_LOG_FILE, true, false, nullptr},

        {PlainConfig::Jobs::CLI_ENABLE_JOBS, true, false, nullptr},
        {PlainConfig::Jobs::CLI_HANDLER_DIR, true, false, nullptr},

        {PlainConfig::Tunneling::CLI_ENABLE_TUNNELING, true, false, nullptr},
        {PlainConfig::Tunneling::CLI_TUNNELING_REGION, true, false, nullptr},
        {PlainConfig::Tunneling::CLI_TUNNELING_SERVICE, true, false, nullptr},
        {PlainConfig::Tunneling::CLI_TUNNELING_DISABLE_NOTIFICATION, false, false, nullptr},

        {PlainConfig::DeviceDefender::CLI_ENABLE_DEVICE_DEFENDER, true, false, nullptr},
        {PlainConfig::DeviceDefender::CLI_DEVICE_DEFENDER_INTERVAL, true, false, nullptr},

        {PlainConfig::FleetProvisioning::CLI_ENABLE_FLEET_PROVISIONING, true, false, nullptr},
        {PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_TEMPLATE_NAME, true, false, nullptr},
        {PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_TEMPLATE_PARAMETERS, true, false, nullptr},
        {PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_CSR_FILE, true, false, nullptr},
        {PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_DEVICE_KEY, true, false, nullptr},

        {PlainConfig::PubSub::CLI_ENABLE_PUB_SUB, true, false, nullptr},
        {PlainConfig::PubSub::CLI_PUB_SUB_PUBLISH_TOPIC, true, false, nullptr},
        {PlainConfig::PubSub::CLI_PUB_SUB_PUBLISH_FILE, true, false, nullptr},
        {PlainConfig::PubSub::CLI_PUB_SUB_SUBSCRIBE_TOPIC, true, false, nullptr},
        {PlainConfig::PubSub::CLI_PUB_SUB_SUBSCRIBE_FILE, true, false, nullptr},

        {PlainConfig::SampleShadow::CLI_ENABLE_SAMPLE_SHADOW, true, false, nullptr},
        {PlainConfig::SampleShadow::CLI_SAMPLE_SHADOW_NAME, true, false, nullptr},
        {PlainConfig::SampleShadow::CLI_SAMPLE_SHADOW_INPUT_FILE, true, false, nullptr},
        {PlainConfig::SampleShadow::CLI_SAMPLE_SHADOW_OUTPUT_FILE, true, false, nullptr},

        {PlainConfig::ConfigShadow::CLI_ENABLE_CONFIG_SHADOW, true, false, nullptr}};

    map<string, ArgumentDefinition> argumentDefinitionMap;
    for (auto &i : argumentDefinitions)
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

        if (search->second.stopIfFound)
        {
            return false;
        }
    }
    return true;
}

bool Config::init(const CliArgs &cliArgs)
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

    if (bReadConfigFile && !ParseConfigFile(filename, false))
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

    if (ParseConfigFile(Config::DEFAULT_FLEET_PROVISIONING_RUNTIME_CONFIG_FILE, true) &&
        ValidateAndStoreRuntimeConfig())
    {
        LOGM_INFO(
            TAG,
            "Successfully fetched Runtime config file '%s' and validated its content.",
            Config::DEFAULT_FLEET_PROVISIONING_RUNTIME_CONFIG_FILE);
    }

    return config.Validate();
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

bool Config::ParseConfigFile(const string &file, bool isRuntimeConfig)
{
    string expandedPath = FileUtils::ExtractExpandedPath(file.c_str());
    if (!FileUtils::FileExists(expandedPath))
    {
        if (!isRuntimeConfig)
        {
            LOGM_DEBUG(TAG, "Unable to open config file %s, file does not exist", Sanitize(expandedPath).c_str());
        }
        else
        {
            LOG_DEBUG(
                TAG,
                "Did not find a runtime configuration file, assuming Fleet Provisioning has not run for this device");
        }

        return false;
    }

    size_t incomingFileSize = FileUtils::GetFileSize(file);
    if (5000 < incomingFileSize)
    {
        LOGM_WARN(
            TAG,
            "Refusing to open config file %s, file size %zu bytes is greater than allowable limit of %zu bytes",
            Sanitize(file).c_str(),
            incomingFileSize,
            5000);
        return false;
    }

    string configFileParentDir = FileUtils::ExtractParentDirectory(expandedPath.c_str());
    FileUtils::ValidateFilePermissions(configFileParentDir, Permissions::CONFIG_DIR, false);
    FileUtils::ValidateFilePermissions(expandedPath.c_str(), Permissions::CONFIG_FILE, false);

    ifstream setting(expandedPath.c_str());
    if (!setting.is_open())
    {
        LOGM_ERROR(TAG, "Unable to open file: '%s'", Sanitize(expandedPath).c_str());
        return false;
    }

    std::string contents((std::istreambuf_iterator<char>(setting)), std::istreambuf_iterator<char>());
    Crt::JsonObject jsonObj = Aws::Crt::JsonObject(contents.c_str());
    if (!jsonObj.WasParseSuccessful())
    {
        LOGM_ERROR(
            TAG, "Couldn't parse JSON config file. GetErrorMessage returns: %s", jsonObj.GetErrorMessage().c_str());
        return false;
    }
    Aws::Crt::JsonView jsonView = Aws::Crt::JsonView(jsonObj);
    config.LoadFromJson(jsonView);

    LOGM_INFO(TAG, "Successfully fetched JSON config file: %s", Sanitize(contents).c_str());
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
        "%s [true|false]:\t\t\t\t\t\tEnables/Disables Sample Shadow feature\n"
        "%s [true|false]:\t\t\t\t\t\tEnables/Disables Config Shadow feature\n"
        "%s <endpoint-value>:\t\t\t\t\t\tUse Specified Endpoint\n"
        "%s <Cert-Location>:\t\t\t\t\t\t\tUse Specified Cert file\n"
        "%s <Key-Location>:\t\t\t\t\t\t\tUse Specified Key file\n"
        "%s <Root-CA-Location>:\t\t\t\t\t\tUse Specified Root-CA file\n"
        "%s <thing-name-value/client-id-value>:\t\t\t\t\tUse Specified Thing Name (Also used as Client ID)\n"
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
        "%s <shadow-name>:\t\t\t\t\tThe name of shadow SampleShadow feature will create or update\n"
        "%s <shadow-input-file>:\t\t\t\t\tThe file the Sample Shadow feature will read from when updating shadow data\n"
        "%s <shadow-output-file>:\t\t\t\t\tThe file the Sample Shadow feature will write the latest shadow document "
        "to\n";

    cout << FormatMessage(
        helpMessageTemplate,
        CLI_HELP,
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
        PlainConfig::SampleShadow::CLI_SAMPLE_SHADOW_OUTPUT_FILE);
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
        PlainConfig::ConfigShadow::JSON_ENABLE_CONFIG_SHADOW);

    clientConfig.close();
    LOGM_INFO(TAG, "Exported settings to: %s", Sanitize(file).c_str());

    chmod(file.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    FileUtils::ValidateFilePermissions(file.c_str(), Permissions::CONFIG_FILE, false);
    return true;
}
