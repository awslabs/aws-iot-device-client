// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "Config.h"
#include "../jobs/JobsFeature.h"
#include "../logging/LoggerFactory.h"
#include "../tunneling/SecureTunnelingFeature.h"
#include <aws/crt/JsonObject.h>
#include <iostream>
#include <map>

using namespace std;
using namespace Aws::Iot;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::SecureTunneling;
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
constexpr char PlainConfig::JSON_KEY_JOBS[];
constexpr char PlainConfig::JSON_KEY_TUNNELING[];
constexpr char PlainConfig::JSON_KEY_DEVICE_DEFENDER[];
constexpr char PlainConfig::JSON_KEY_FLEET_PROVISIONING[];
constexpr char PlainConfig::JSON_KEY_RUNTIME_CONFIG[];

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
        cert = json.GetString(jsonKey).c_str();
    }

    jsonKey = JSON_KEY_KEY;
    if (json.ValueExists(jsonKey))
    {
        key = json.GetString(jsonKey).c_str();
    }

    jsonKey = JSON_KEY_ROOT_CA;
    if (json.ValueExists(jsonKey))
    {
        rootCa = json.GetString(jsonKey).c_str();
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
        RuntimeConfig temp;
        temp.LoadFromJson(json.GetJsonObject(jsonKey));
        runtimeConfig = temp;
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
        cert = cliArgs.at(PlainConfig::CLI_CERT).c_str();
    }
    if (cliArgs.count(PlainConfig::CLI_KEY))
    {
        key = cliArgs.at(PlainConfig::CLI_KEY).c_str();
    }
    if (cliArgs.count(PlainConfig::CLI_ROOT_CA))
    {
        rootCa = cliArgs.at(PlainConfig::CLI_ROOT_CA).c_str();
    }
    if (cliArgs.count(PlainConfig::CLI_THING_NAME))
    {
        thingName = cliArgs.at(PlainConfig::CLI_THING_NAME).c_str();
    }

    return jobs.LoadFromCliArgs(cliArgs) && tunneling.LoadFromCliArgs(cliArgs) &&
           deviceDefender.LoadFromCliArgs(cliArgs) && fleetProvisioning->LoadFromCliArgs(cliArgs);
}

bool PlainConfig::Validate() const
{
    if (!endpoint.has_value() || endpoint->empty())
    {
        LOG_ERROR(Config::TAG, "*** AWS IOT DEVICE CLIENT FATAL ERROR: Endpoint is missing ***");
        return false;
    }
    if (!cert.has_value() || cert->empty())
    {
        LOG_ERROR(Config::TAG, "*** AWS IOT DEVICE CLIENT FATAL ERROR: Certificate is missing ***");
        return false;
    }
    if (!key.has_value() || key->empty())
    {
        LOG_ERROR(Config::TAG, "*** AWS IOT DEVICE CLIENT FATAL ERROR: Private Key is missing ***");
        return false;
    }
    if (!rootCa.has_value() || rootCa->empty())
    {
        LOG_ERROR(Config::TAG, "*** AWS IOT DEVICE CLIENT FATAL ERROR: Root CA is missing ***");
        return false;
    }
    if (!thingName.has_value() || thingName->empty())
    {
        LOG_ERROR(Config::TAG, "*** AWS IOT DEVICE CLIENT FATAL ERROR: Thing name is missing ***");
        return false;
    }
    if (!jobs.Validate())
    {
        return false;
    }
    if (!tunneling.Validate())
    {
        return false;
    }
    if (!deviceDefender.Validate())
    {
        return false;
    }
    if (fleetProvisioning && !fleetProvisioning->Validate())
    {
        return false;
    }
    return true;
}

constexpr char PlainConfig::Jobs::CLI_ENABLE_JOBS[];
constexpr char PlainConfig::Jobs::JSON_KEY_ENABLED[];

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

    return true;
}

bool PlainConfig::Jobs::Validate() const
{
    return true;
}

constexpr char PlainConfig::Tunneling::CLI_ENABLE_TUNNELING[];
constexpr char PlainConfig::Tunneling::CLI_TUNNELING_DESTINATION_ACCESS_TOKEN[];
constexpr char PlainConfig::Tunneling::CLI_TUNNELING_REGION[];
constexpr char PlainConfig::Tunneling::CLI_TUNNELING_SERVICE[];
constexpr char PlainConfig::Tunneling::CLI_TUNNELING_DISABLE_NOTIFICATION[];

constexpr char PlainConfig::Tunneling::JSON_KEY_ENABLED[];
constexpr char PlainConfig::Tunneling::JSON_KEY_DESTINATION_ACCESS_TOKEN[];
constexpr char PlainConfig::Tunneling::JSON_KEY_REGION[];
constexpr char PlainConfig::Tunneling::JSON_KEY_PORT[];
constexpr char PlainConfig::Tunneling::JSON_KEY_SUBSCRIBE_NOTIFICATION[];

bool PlainConfig::Tunneling::LoadFromJson(const Crt::JsonView &json)
{
    const char *jsonKey = JSON_KEY_ENABLED;
    if (json.ValueExists(jsonKey))
    {
        enabled = json.GetBool(jsonKey);
    }

    jsonKey = JSON_KEY_DESTINATION_ACCESS_TOKEN;
    if (json.ValueExists(jsonKey))
    {
        destinationAccessToken = json.GetString(jsonKey).c_str();
    }

    jsonKey = JSON_KEY_REGION;
    if (json.ValueExists(jsonKey))
    {
        region = json.GetString(jsonKey).c_str();
    }

    jsonKey = JSON_KEY_PORT;
    if (json.ValueExists(jsonKey))
    {
        port = json.GetInteger(jsonKey);
    }

    jsonKey = JSON_KEY_SUBSCRIBE_NOTIFICATION;
    if (json.ValueExists(jsonKey))
    {
        subscribeNotification = json.GetBool(jsonKey);
    }

    return true;
}

bool PlainConfig::Tunneling::LoadFromCliArgs(const CliArgs &cliArgs)
{
    if (cliArgs.count(PlainConfig::Tunneling::CLI_ENABLE_TUNNELING))
    {
        enabled = true;
    }
    if (cliArgs.count(PlainConfig::Tunneling::CLI_TUNNELING_DESTINATION_ACCESS_TOKEN))
    {
        destinationAccessToken = cliArgs.at(PlainConfig::Tunneling::CLI_TUNNELING_DESTINATION_ACCESS_TOKEN).c_str();
    }
    if (cliArgs.count(PlainConfig::Tunneling::CLI_TUNNELING_REGION))
    {
        region = cliArgs.at(PlainConfig::Tunneling::CLI_TUNNELING_REGION).c_str();
    }
    if (cliArgs.count(PlainConfig::Tunneling::CLI_TUNNELING_SERVICE))
    {
        auto service = cliArgs.at(PlainConfig::Tunneling::CLI_TUNNELING_SERVICE);
        port = SecureTunnelingFeature::GetPortFromService(service);
    }
    if (cliArgs.count(PlainConfig::Tunneling::CLI_TUNNELING_DISABLE_NOTIFICATION))
    {
        subscribeNotification = false;
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
        LOG_ERROR(Config::TAG, "*** AWS IOT DEVICE CLIENT FATAL ERROR: destination-access-token is missing ***");
        return false;
    }
    if (!region.has_value() || region->empty())
    {
        LOG_ERROR(Config::TAG, "*** AWS IOT DEVICE CLIENT FATAL ERROR: region is missing ***");
        return false;
    }
    if (!port.has_value() || !SecureTunnelingFeature::IsValidPort(port.value()))
    {
        LOG_ERROR(Config::TAG, "*** AWS IOT DEVICE CLIENT FATAL ERROR: port is missing or invalid ***");
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
                "*** AWS IOT DEVICE CLIENT FATAL ERROR: Failed to convert CLI argument {%s} to integer, please use a "
                "valid integer between 1 and MAX_INT ***",
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
        LOG_ERROR(Config::TAG, "*** AWS IOT DEVICE CLIENT FATAL ERROR: Interval value <= 0 ***");
        return false;
    }

    return true;
}

constexpr char PlainConfig::FleetProvisioning::CLI_ENABLE_FLEET_PROVISIONING[];
constexpr char PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_TEMPLATE_NAME[];

constexpr char PlainConfig::FleetProvisioning::JSON_KEY_ENABLED[];
constexpr char PlainConfig::FleetProvisioning::JSON_KEY_TEMPLATE_NAME[];

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
        templateName = json.GetString(jsonKey).c_str();
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

    return true;
}

bool PlainConfig::FleetProvisioning::Validate() const
{
    if (!enabled)
    {
        return true;
    }

    return templateName.has_value();
}

constexpr char PlainConfig::RuntimeConfig::JSON_KEY_COMPLETED_FLEET_PROVISIONING[];
constexpr char PlainConfig::RuntimeConfig::JSON_KEY_CERT[];
constexpr char PlainConfig::RuntimeConfig::JSON_KEY_KEY[];
constexpr char PlainConfig::RuntimeConfig::JSON_KEY_THING_NAME[];

bool PlainConfig::RuntimeConfig::LoadFromJson(const Crt::JsonView &json)
{
    const char *jsonKey = JSON_KEY_COMPLETED_FLEET_PROVISIONING;
    if (json.ValueExists(jsonKey))
    {
        completedFleetProvisioning = json.GetBool(jsonKey);
    }

    jsonKey = JSON_KEY_CERT;
    if (json.ValueExists(jsonKey))
    {
        cert = json.GetString(jsonKey).c_str();
    }

    jsonKey = JSON_KEY_KEY;
    if (json.ValueExists(jsonKey))
    {
        key = json.GetString(jsonKey).c_str();
    }

    jsonKey = JSON_KEY_THING_NAME;
    if (json.ValueExists(jsonKey))
    {
        thingName = json.GetString(jsonKey).c_str();
    }

    return true;
}
bool PlainConfig::RuntimeConfig::LoadFromCliArgs(const CliArgs &cliArgs)
{
    /*
     * No Command line arguments for Runtime Config
     */
    return true;
}

bool PlainConfig::RuntimeConfig::Validate() const
{
    if (!completedFleetProvisioning.has_value() || !completedFleetProvisioning.value())
    {
        return false;
    }
    return cert.has_value() && key.has_value() && thingName.has_value();
}

constexpr char Config::TAG[];
constexpr char Config::DEFAULT_CONFIG_FILE[];
constexpr char Config::JOBS_HANDLER_DIR[];
constexpr char Config::CLI_HELP[];
constexpr char Config::CLI_EXPORT_DEFAULT_SETTINGS[];
constexpr char Config::CLI_CONFIG_FILE[];
constexpr char Config::DEFAULT_RUNTIME_CONFIG_FILE[];

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

        {PlainConfig::Jobs::CLI_ENABLE_JOBS, false, false, nullptr},

        {PlainConfig::Tunneling::CLI_ENABLE_TUNNELING, false, false, nullptr},
        {PlainConfig::Tunneling::CLI_TUNNELING_DESTINATION_ACCESS_TOKEN, true, false, nullptr},
        {PlainConfig::Tunneling::CLI_TUNNELING_REGION, true, false, nullptr},
        {PlainConfig::Tunneling::CLI_TUNNELING_SERVICE, true, false, nullptr},
        {PlainConfig::Tunneling::CLI_TUNNELING_DISABLE_NOTIFICATION, false, false, nullptr},

        {PlainConfig::DeviceDefender::CLI_ENABLE_DEVICE_DEFENDER, false, false, nullptr},
        {PlainConfig::DeviceDefender::CLI_DEVICE_DEFENDER_INTERVAL, true, false, nullptr},

        {PlainConfig::FleetProvisioning::CLI_ENABLE_FLEET_PROVISIONING, false, false, nullptr},
        {PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_TEMPLATE_NAME, true, false, nullptr},
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
                "*** AWS IOT DEVICE CLIENT FATAL ERROR: Unrecognised command line argument: %s ***",
                currentArg.c_str());
            return false;
        }

        if (cliArgs.find(currentArg) != cliArgs.end())
        {
            LOGM_ERROR(
                TAG,
                "*** AWS IOT DEVICE CLIENT FATAL ERROR: Command Line argument '%s' cannot be specified more than once "
                "***",
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
                    "*** AWS IOT DEVICE CLIENT FATAL ERROR: Command Line argument '%s' was passed without specifying "
                    "addition argument "
                    "***",
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
    if (cliArgs.count(Config::CLI_CONFIG_FILE))
    {
        if (!ParseConfigFile(cliArgs.at(Config::CLI_CONFIG_FILE)))
        {
            return false;
        }
    }
    else
    {
        if (!ParseConfigFile(Config::DEFAULT_CONFIG_FILE))
        {
            return false;
        }
    }

    config.LoadFromCliArgs(cliArgs);

    if (ParseConfigFile(Config::DEFAULT_RUNTIME_CONFIG_FILE) && ValidateAndStoreRuntimeConfig())
    {
        LOGM_INFO(
            TAG,
            "Successfully fetched Runtime config file '%s' and validated its content.",
            Config::DEFAULT_RUNTIME_CONFIG_FILE);
    }

    return config.Validate();
}

bool Config::ValidateAndStoreRuntimeConfig()
{
    // check if all values are present and also check if the files are present then only overwrite values

    if (config.runtimeConfig->Validate())
    {
        config.cert = config.runtimeConfig->cert.value().c_str();
        config.key = config.runtimeConfig->key.value().c_str();
        config.thingName = config.runtimeConfig->thingName.value().c_str();
        return true;
    }
    LOGM_WARN(
        TAG, "Failed to Validate runtime configurations. Please check '%s' file", Config::DEFAULT_RUNTIME_CONFIG_FILE);
    return false;
}

bool Config::ParseConfigFile(const string &file)
{
    ifstream setting(file);
    if (!setting.is_open())
    {
        LOGM_ERROR(TAG, "*** AWS IOT DEVICE CLIENT FATAL ERROR: Unable to open file: '%s' ***", file.c_str());
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
        "%s:\t\t\t\t\t\t\t\tEnables Jobs feature\n"
        "%s:\t\t\t\t\t\t\tEnables Tunneling feature\n"
        "%s:\t\t\t\t\t\tEnables Device Defender feature\n"
        "%s:\t\t\t\t\t\tEnables Fleet Provisioning feature\n"
        "%s <endpoint-value>:\t\t\t\t\t\tUse Specified Endpoint\n"
        "%s <Cert-Location>:\t\t\t\t\t\t\tUse Specified Cert file\n"
        "%s <Key-Location>:\t\t\t\t\t\t\tUse Specified Key file\n"
        "%s <Root-CA-Location>:\t\t\t\t\t\tUse Specified Root-CA file\n"
        "%s <thing-name-value>:\t\t\t\t\tUse Specified Thing Name\n"
        "%s <destination-access-token>:\tUse Specified Destination Access Token\n"
        "%s <region>:\t\t\t\t\t\tUse Specified AWS Region\n"
        "%s <service>:\t\t\t\t\t\tConnect secure tunnel to specific service\n"
        "%s:\t\t\t\t\tDisable MQTT new tunnel notification\n"
        "%s <interval-in-seconds>:\t\t\tPositive integer to publish Device Defender metrics\n"
        "%s <template-name>:\t\t\tUse specified Fleet Provisioning template name\n";

    cout << FormatMessage(
        helpMessageTemplate,
        CLI_HELP,
        CLI_EXPORT_DEFAULT_SETTINGS,
        CLI_CONFIG_FILE,
        PlainConfig::Jobs::CLI_ENABLE_JOBS,
        PlainConfig::Tunneling::CLI_ENABLE_TUNNELING,
        PlainConfig::DeviceDefender::CLI_ENABLE_DEVICE_DEFENDER,
        PlainConfig::FleetProvisioning::CLI_ENABLE_FLEET_PROVISIONING,
        PlainConfig::CLI_ENDPOINT,
        PlainConfig::CLI_CERT,
        PlainConfig::CLI_KEY,
        PlainConfig::CLI_ROOT_CA,
        PlainConfig::CLI_THING_NAME,
        PlainConfig::Tunneling::CLI_TUNNELING_DESTINATION_ACCESS_TOKEN,
        PlainConfig::Tunneling::CLI_TUNNELING_REGION,
        PlainConfig::Tunneling::CLI_TUNNELING_SERVICE,
        PlainConfig::Tunneling::CLI_TUNNELING_DISABLE_NOTIFICATION,
        PlainConfig::DeviceDefender::CLI_DEVICE_DEFENDER_INTERVAL,
        PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_TEMPLATE_NAME);
}

void Config::ExportDefaultSetting(const string &file)
{
    string jsonTemplate = R"({
    "%s": "<replace_with_endpoint_value>",
    "%s": "<replace_with_certificate_file_location>",
    "%s": "<replace_with_private_key_file_location>",
    "%s": "<replace_with_root_ca_file_location>",
    "%s": "<replace_with_thing_name>",
    "%s": {
        "%s": true
    },
    "%s": {
        "%s": true,
        "%s": "<replace_with_destination_access_token>",
        "%s": "<replace_with_region>",
        "%s": <replace_with_port>,
        "%s": true
    },
    "%s": {
        "%s": true,
        "%s": <replace_with_interval>
    }
    "%s": {
        "%s": true,
        "%s": "<replace_with_template_name>"
    }
}
)";
    ofstream clientConfig;
    clientConfig.open(file);
    clientConfig << FormatMessage(
        jsonTemplate.c_str(),
        PlainConfig::JSON_KEY_ENDPOINT,
        PlainConfig::JSON_KEY_CERT,
        PlainConfig::JSON_KEY_KEY,
        PlainConfig::JSON_KEY_ROOT_CA,
        PlainConfig::JSON_KEY_THING_NAME,
        PlainConfig::JSON_KEY_JOBS,
        PlainConfig::Jobs::JSON_KEY_ENABLED,
        PlainConfig::JSON_KEY_TUNNELING,
        PlainConfig::Tunneling::JSON_KEY_ENABLED,
        PlainConfig::Tunneling::JSON_KEY_DESTINATION_ACCESS_TOKEN,
        PlainConfig::Tunneling::JSON_KEY_REGION,
        PlainConfig::Tunneling::JSON_KEY_PORT,
        PlainConfig::Tunneling::JSON_KEY_SUBSCRIBE_NOTIFICATION,
        PlainConfig::JSON_KEY_DEVICE_DEFENDER,
        PlainConfig::DeviceDefender::JSON_KEY_ENABLED,
        PlainConfig::DeviceDefender::JSON_KEY_INTERVAL,
        PlainConfig::JSON_KEY_FLEET_PROVISIONING,
        PlainConfig::FleetProvisioning::JSON_KEY_ENABLED,
        PlainConfig::FleetProvisioning::JSON_KEY_TEMPLATE_NAME);

    clientConfig.close();
    LOGM_INFO(TAG, "Exported settings to: %s", file.c_str());
}
