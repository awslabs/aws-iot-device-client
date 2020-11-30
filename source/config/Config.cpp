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

const char *Config::DEFAULT_CONFIG_FILE = "/etc/aws-iot-device-client.conf";
const char *Config::CONFIG_FILE = "config-file";
const char *Config::JOBS_FEATURE = "jobs";
const char *Config::JOBS_HANDLER_DIR = "handler_directory";
const char *Config::TUNNELING_FEATURE = "tunneling";
const char *Config::THING_NAME = "thing_name";
const char *Config::ENDPOINT = "endpoint";
const char *Config::CERTIFICATE = "cert";
const char *Config::PRIVATE_KEY = "key";
const char *Config::ROOT_CA = "root-ca";
const char *Config::FEATURE_ENABLED = "enabled";
/**
 * Store settings passed via JSON config file in @jsonObj JsonObject.
 */
bool Config::ParseConfigFile(string file)
{
    ifstream dcSetting(file);
    if (!dcSetting.is_open())
    {
        LOGM_ERROR(TAG, "*** DC FATAL ERROR: Unable to open file: '%s' ***", file.c_str());
        return false;
    }

    std::string contents((std::istreambuf_iterator<char>(dcSetting)), std::istreambuf_iterator<char>());
    Aws::Crt::String jsonConfigFile = contents.c_str();
    jsonObj = Aws::Crt::JsonObject(jsonConfigFile);
    if (!jsonObj.WasParseSuccessful())
    {
        LOGM_ERROR(
            TAG, "Couldn't parse JSON config file. GetErrorMessage returns: %s", jsonObj.GetErrorMessage().c_str());
        return false;
    }

    LOGM_INFO(TAG, "Successfully fetched JSON config file: %s", contents.c_str());
    dcSetting.close();
    return true;
}

/**
 * Store settings passed via command line argument in @jsonObj JsonObject and validates if all required  settings are
 * passed either via command line or JSON Config file. Converts final @jsonObj JsonObject into @dcConfig JsonView. This
 * JsonView object will be passed across the DC program to fetch settings.
 */
int Config::StoreAndValidate(map<string, string> *cliArgs)
{
    if (cliArgs->count(Config::ENDPOINT))
    {
        jsonObj.WithString(Config::ENDPOINT, cliArgs->at(Config::ENDPOINT).c_str());
    }
    if (cliArgs->count(Config::CERTIFICATE))
    {
        jsonObj.WithString(Config::CERTIFICATE, cliArgs->at(Config::CERTIFICATE).c_str());
    }
    if (cliArgs->count(Config::PRIVATE_KEY))
    {
        jsonObj.WithString(Config::PRIVATE_KEY, cliArgs->at(Config::PRIVATE_KEY).c_str());
    }
    if (cliArgs->count(Config::ROOT_CA))
    {
        jsonObj.WithString(Config::ROOT_CA, cliArgs->at(Config::ROOT_CA).c_str());
    }
    if (cliArgs->count(Config::THING_NAME))
    {
        jsonObj.WithString(Config::THING_NAME, cliArgs->at(Config::THING_NAME).c_str());
    }
    if (!ValidateJobs(cliArgs))
    {
        return Config::ABORT;
    }
    if (!ValidateTunneling(cliArgs))
    {
        return Config::ABORT;
    }

    dcConfig = Aws::Crt::JsonView(jsonObj);

    if (!dcConfig.KeyExists(Config::ENDPOINT)  || dcConfig.GetString(Config::ENDPOINT).empty())
    {
        LOG_ERROR(TAG, "*** DC FATAL ERROR: Endpoint is missing ***");
        return Config::ABORT;
    }
    if (!dcConfig.KeyExists(Config::CERTIFICATE)  || dcConfig.GetString(Config::CERTIFICATE).empty())
    {
        LOG_ERROR(TAG, "*** DC FATAL ERROR: Certificate is missing ***");
        return Config::ABORT;
    }
    if (!dcConfig.KeyExists(Config::PRIVATE_KEY)  || dcConfig.GetString(Config::PRIVATE_KEY).empty())
    {
        LOG_ERROR(TAG, "*** DC FATAL ERROR: Private Key is missing ***");
        return Config::ABORT;
    }
    if (!dcConfig.KeyExists(Config::ROOT_CA) || dcConfig.GetString(Config::ROOT_CA).empty())
    {
        LOG_ERROR(TAG, "*** DC FATAL ERROR: Root CA is missing ***");
        return Config::ABORT;
    }
    if (!dcConfig.KeyExists(Config::THING_NAME) || dcConfig.GetString(Config::THING_NAME).empty())
    {
        LOG_ERROR(TAG, "*** DC FATAL ERROR: Thing name is missing ***");
        return Config::ABORT;
    }
    return Config::SUCCESS;
}

/**
 * Validate and store settings used for starting Jobs feature
 */
bool Config::ValidateJobs(map<string, string> *cliArgs)
{
    dcConfig = Aws::Crt::JsonView(jsonObj);
    if ((cliArgs->count(Config::JOBS_FEATURE)) ||
        dcConfig.GetJsonObject(Config::JOBS_FEATURE).GetString(Config::FEATURE_ENABLED) == "true")
    {
        /**
        TODO: Delete this Comment block before making the project public.
        For Future reference. This way we can validate feature specific required setting.

        if(!dcConfig.GetJsonObject(Config::JOBS_FEATURE).KeyExists(Config::SOME_REQUIRED_FEATURE)){
             LOG_ERROR(TAG, "*** DC FATAL ERROR: SOME_REQUIRED_FEATURE for Jobs feature is missing ***");
             return false;
         }
         */
        jsonObj.WithString(Config::JOBS_FEATURE, "true");
    }
    return true;
}

/**
 * Validate and store settings used for starting Tunneling feature
 */
bool Config::ValidateTunneling(map<string, string> *cliArgs)
{
    dcConfig = Aws::Crt::JsonView(jsonObj);
    if ((cliArgs->count(Config::TUNNELING_FEATURE)) ||
        dcConfig.GetJsonObject(Config::TUNNELING_FEATURE).GetString(Config::FEATURE_ENABLED) == "true")
    {
        // TODO: Validate required config for Tunneling features. Currently no required Config is present for Tunneling
        // feature.
        jsonObj.WithString(Config::TUNNELING_FEATURE, "true");
    }
    return true;
}

/**
 * This method is responsible for initializing Config object. It parses configurations/settings from JSON Config file
 * and Command Line argument and validates it. After validation it stores it in an JsonObject. @dcConfig JsonView object
 * will be passed across the DC program to fetch settings.
 */
int Config::init(map<string, string> *cliArgs)
{
    if (cliArgs->count(Config::CONFIG_FILE))
    {
        if (!ParseConfigFile(cliArgs->at(Config::CONFIG_FILE)))
        {
            return Config::ABORT;
        }
    }
    else
    {
        if (!ParseConfigFile(Config::DEFAULT_CONFIG_FILE))
        {
            return Config::ABORT;
        }
    }
    return StoreAndValidate(cliArgs);
}

/**
 * Helper methods
 */

bool Config::isFeatureEnabled(Aws::Crt::String featureName)
{
    return dcConfig.KeyExists(featureName) && dcConfig.GetString(featureName) == "true";
}