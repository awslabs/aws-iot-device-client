// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_CONFIG_H
#define DEVICE_CLIENT_CONFIG_H

#include <aws/crt/JsonObject.h>

#include <fstream>
#include <map>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            class Config
            {
              private:
                const char *TAG = "Config.cpp";
                Crt::JsonObject jsonObj;
                int StoreAndValidate(std::map<std::string, std::string> *cliArgs);
                bool ParseConfigFile(std::string file);
                bool ValidateJobs(std::map<std::string, std::string> *cliArgs);
                bool ValidateTunneling(std::map<std::string, std::string> *cliArgs);

              public:
                Config() {}
                ~Config() {}
                static const char *DEFAULT_CONFIG_FILE;
                static const char *CONFIG_FILE;
                static const char *JOBS_FEATURE;
                static const char *JOBS_HANDLER_DIR;
                static const char *TUNNELING_FEATURE;
                static const char *THING_NAME;
                static const char *ENDPOINT;
                static const char *CERTIFICATE;
                static const char *PRIVATE_KEY;
                static const char *ROOT_CA;
                static const char *FEATURE_ENABLED;
                static const int SUCCESS = 0;
                static const int GRACEFUL_SHUTDOWN = 1;
                static const int ABORT = 2;
                Crt::JsonView dcConfig;
                int init(std::map<std::string, std::string> *cliArgs);
                bool isFeatureEnabled(Crt::String featureName);
            };
        } // namespace DeviceClient
    }     // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_CONFIG_H
