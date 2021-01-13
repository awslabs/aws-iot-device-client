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
           deviceDefender.LoadFromCliArgs(cliArgs) && fleetProvisioning.LoadFromCliArgs(cliArgs);
}

bool PlainConfig::LoadFromEnvironment()
{
    return logConfig.LoadFromEnvironment() && jobs.LoadFromEnvironment() && tunneling.LoadFromEnvironment() &&
           deviceDefender.LoadFromEnvironment() && fleetProvisioning.LoadFromEnvironment() &&
           fleetProvisioningRuntimeConfig.LoadFromEnvironment();
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
    if (!key.has_value() || key->empty())
    {
        LOGM_ERROR(Config::TAG, "*** %s: Private Key is missing ***", DeviceClient::DC_FATAL_ERROR);
        return false;
    }
    if (!rootCa.has_value() || rootCa->empty())
    {
        LOGM_ERROR(Config::TAG, "*** %s: Root CA is missing ***", DeviceClient::DC_FATAL_ERROR);
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

int PlainConfig::LogConfig::ParseLogLevel(string level)
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
        throw std::invalid_argument(FormatMessage("Provided log level %s is not a known log level", level.c_str()));
    }
}

string PlainConfig::LogConfig::ParseLogType(string value)
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
            value.c_str(),
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
                logLevel = ParseLogLevel(json.GetString(jsonKey).c_str());
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
                type = ParseLogType(json.GetString(jsonKey).c_str());
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
            file = FileUtils::ExtractExpandedPath(json.GetString(jsonKey).c_str());
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
            logLevel = ParseLogLevel(cliArgs.at(CLI_LOG_LEVEL));
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
            type = ParseLogType(cliArgs.at(CLI_LOG_TYPE));
        }
        catch (const std::invalid_argument &e)
        {
            LOGM_ERROR(Config::TAG, "Unable to parse incoming log type value passed via command line: %s", e.what());
            return false;
        }
    }

    if (cliArgs.count(CLI_LOG_FILE))
    {
        file = FileUtils::ExtractExpandedPath(cliArgs.at(CLI_LOG_FILE).c_str());
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

    return true;
}

bool PlainConfig::Jobs::LoadFromCliArgs(const CliArgs &cliArgs)
{
    if (cliArgs.count(PlainConfig::Jobs::CLI_ENABLE_JOBS))
    {
        enabled = true;
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
        enabled = true;
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
        enabled = true;
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
    if (!interval.has_value() || (interval.value() <= 0))
    {
        LOGM_ERROR(Config::TAG, "*** %s: Interval value <= 0 ***", DeviceClient::DC_FATAL_ERROR);
        return false;
    }

    return true;
}

constexpr char PlainConfig::FleetProvisioning::CLI_ENABLE_FLEET_PROVISIONING[];
constexpr char PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_TEMPLATE_NAME[];
constexpr char PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_CSR_FILE[];

constexpr char PlainConfig::FleetProvisioning::JSON_KEY_ENABLED[];
constexpr char PlainConfig::FleetProvisioning::JSON_KEY_TEMPLATE_NAME[];
constexpr char PlainConfig::FleetProvisioning::JSON_KEY_CSR_FILE[];

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

    return true;
}

bool PlainConfig::FleetProvisioning::LoadFromCliArgs(const CliArgs &cliArgs)
{
    if (cliArgs.count(PlainConfig::FleetProvisioning::CLI_ENABLE_FLEET_PROVISIONING))
    {
        enabled = true;
    }
    if (cliArgs.count(PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_TEMPLATE_NAME))
    {
        templateName = cliArgs.at(PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_TEMPLATE_NAME).c_str();
    }
    if (cliArgs.count(PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_CSR_FILE))
    {
        csrFile = FileUtils::ExtractExpandedPath(
            cliArgs.at(PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_CSR_FILE).c_str());
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
    return true;
}

constexpr char PlainConfig::FleetProvisioningRuntimeConfig::JSON_KEY_COMPLETED_FLEET_PROVISIONING[];
constexpr char PlainConfig::FleetProvisioningRuntimeConfig::JSON_KEY_CERT[];
constexpr char PlainConfig::FleetProvisioningRuntimeConfig::JSON_KEY_KEY[];
constexpr char PlainConfig::FleetProvisioningRuntimeConfig::JSON_KEY_THING_NAME[];

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

constexpr char Config::TAG[];
constexpr char Config::DEFAULT_CONFIG_DIR[];
constexpr char Config::DEFAULT_KEY_DIR[];
constexpr char Config::DEFAULT_CONFIG_FILE[];
constexpr char Config::CLI_HELP[];
constexpr char Config::CLI_EXPORT_DEFAULT_SETTINGS[];
constexpr char Config::CLI_CONFIG_FILE[];
constexpr char Config::DEFAULT_FLEET_PROVISIONING_RUNTIME_CONFIG_FILE[];

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

        {PlainConfig::Jobs::CLI_ENABLE_JOBS, false, false, nullptr},
        {PlainConfig::Jobs::CLI_HANDLER_DIR, true, false, nullptr},

        {PlainConfig::Tunneling::CLI_ENABLE_TUNNELING, false, false, nullptr},
        {PlainConfig::Tunneling::CLI_TUNNELING_REGION, true, false, nullptr},
        {PlainConfig::Tunneling::CLI_TUNNELING_SERVICE, true, false, nullptr},
        {PlainConfig::Tunneling::CLI_TUNNELING_DISABLE_NOTIFICATION, false, false, nullptr},

        {PlainConfig::DeviceDefender::CLI_ENABLE_DEVICE_DEFENDER, false, false, nullptr},
        {PlainConfig::DeviceDefender::CLI_DEVICE_DEFENDER_INTERVAL, true, false, nullptr},

        {PlainConfig::FleetProvisioning::CLI_ENABLE_FLEET_PROVISIONING, false, false, nullptr},
        {PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_TEMPLATE_NAME, true, false, nullptr},
        {PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_CSR_FILE, true, false, nullptr},
    };

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
                currentArg.c_str());
            return false;
        }

        if (cliArgs.find(currentArg) != cliArgs.end())
        {
            LOGM_ERROR(
                TAG,
                "*** %s: Command Line argument '%s' cannot be specified more than once ***",
                DeviceClient::DC_FATAL_ERROR,
                currentArg.c_str());
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
                    currentArg.c_str());
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
    if (cliArgs.count(Config::CLI_CONFIG_FILE))
    {
        filename = cliArgs.at(Config::CLI_CONFIG_FILE);
    }

    if (!ParseConfigFile(filename, false))
    {
        LOGM_ERROR(
            TAG, "*** %s: Unable to Parse Config file: '%s' ***", DeviceClient::DC_FATAL_ERROR, filename.c_str());
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
    struct stat info;
    if (stat(expandedPath.c_str(), &info) != 0)
    {
        if (!isRuntimeConfig)
        {
            LOGM_DEBUG(TAG, "Unable to open config file %s, file does not exist", expandedPath.c_str());
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
            file.c_str(),
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
        LOGM_ERROR(TAG, "Unable to open file: '%s'", expandedPath.c_str());
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

    LOGM_INFO(TAG, "Successfully fetched JSON config file: %s", contents.c_str());
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
        "%s:\t\t\t\t\t\t\t\tEnables Jobs feature\n"
        "%s:\t\t\t\t\t\t\tEnables Tunneling feature\n"
        "%s:\t\t\t\t\t\tEnables Device Defender feature\n"
        "%s:\t\t\t\t\t\tEnables Fleet Provisioning feature\n"
        "%s <endpoint-value>:\t\t\t\t\t\tUse Specified Endpoint\n"
        "%s <Cert-Location>:\t\t\t\t\t\t\tUse Specified Cert file\n"
        "%s <Key-Location>:\t\t\t\t\t\t\tUse Specified Key file\n"
        "%s <Root-CA-Location>:\t\t\t\t\t\tUse Specified Root-CA file\n"
        "%s <thing-name-value>:\t\t\t\t\tUse Specified Thing Name\n"
        "%s <Jobs-handler-directory>:\t\t\t\tUse specified directory to find job handlers\n"
        "%s <region>:\t\t\t\t\t\tUse Specified AWS Region for Secure Tunneling\n"
        "%s <service>:\t\t\t\t\t\tConnect secure tunnel to specific service\n"
        "%s:\t\t\t\t\tDisable MQTT new tunnel notification for Secure Tunneling\n"
        "%s <interval-in-seconds>:\t\t\tPositive integer to publish Device Defender metrics\n"
        "%s <template-name>:\t\t\tUse specified Fleet Provisioning template name\n"
        "%s <csr-file-path>:\t\t\t\t\t\tUse specified CSR file to generate a certificate by keeping user private key "
        "secure. "
        "If CSR file is not provided, Client will use Claim Certificate and Private key to generate new Certificate "
        "and Private Key while provisioning the device\n";

    cout << FormatMessage(
        helpMessageTemplate,
        CLI_HELP,
        CLI_EXPORT_DEFAULT_SETTINGS,
        CLI_CONFIG_FILE,
        PlainConfig::LogConfig::CLI_LOG_LEVEL,
        PlainConfig::LogConfig::CLI_LOG_TYPE,
        PlainConfig::LogConfig::CLI_LOG_FILE,
        PlainConfig::Jobs::CLI_ENABLE_JOBS,
        PlainConfig::Tunneling::CLI_ENABLE_TUNNELING,
        PlainConfig::DeviceDefender::CLI_ENABLE_DEVICE_DEFENDER,
        PlainConfig::FleetProvisioning::CLI_ENABLE_FLEET_PROVISIONING,
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
        PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_CSR_FILE);
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
        "%s": true,
        "%s": "<replace_with_job_handler-directory_path>"
    },
    "%s": {
        "%s": true
    },
    "%s": {
        "%s": true,
        "%s": <replace_with_interval>
    }
    "%s": {
        "%s": true,
        "%s": "<replace_with_template_name>",
        "%s": "<replace_with_csr-file-path>"
    }
}
)";
    ofstream clientConfig(file);
    if (!clientConfig.is_open())
    {
        LOGM_ERROR(TAG, "Unable to open file: '%s'", file.c_str());
        return false;
    }
    clientConfig << FormatMessage(
        jsonTemplate.c_str(),
        PlainConfig::JSON_KEY_ENDPOINT,
        PlainConfig::JSON_KEY_CERT,
        PlainConfig::JSON_KEY_KEY,
        PlainConfig::JSON_KEY_ROOT_CA,
        PlainConfig::JSON_KEY_THING_NAME,
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
        PlainConfig::FleetProvisioning::JSON_KEY_CSR_FILE);

    clientConfig.close();
    LOGM_INFO(TAG, "Exported settings to: %s", file.c_str());

    chmod(file.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    FileUtils::ValidateFilePermissions(file.c_str(), Permissions::CONFIG_FILE, false);
    return true;
}
