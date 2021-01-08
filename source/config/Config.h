// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_CONFIG_H
#define DEVICE_CLIENT_CONFIG_H

#include <aws/crt/JsonObject.h>
#include <aws/crt/Optional.h>
#include <fstream>
#include <map>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            using CliArgs = std::map<std::string, std::string>;
            static constexpr char DC_FATAL_ERROR[] = "AWS IOT DEVICE CLIENT FATAL ERROR";

            class LoadableFromJsonAndCli
            {
              public:
                virtual bool LoadFromJson(const Crt::JsonView &json) = 0;
                virtual bool LoadFromCliArgs(const CliArgs &cliArgs) = 0;
                virtual bool Validate() const = 0;
            };

            /**
             * \brief Default permission values
             */
            struct Permissions
            {
                /** Directories **/
                static constexpr int KEY_DIR = 700;
                static constexpr int ROOT_CA_DIR = 700;
                static constexpr int CERT_DIR = 700;
                static constexpr int CSR_DIR = 700;
                static constexpr int CONFIG_DIR = 745;
                static constexpr int LOG_DIR = 745;

                /** Files **/
                static constexpr int PRIVATE_KEY = 600;
                static constexpr int PUBLIC_CERT = 644;
                static constexpr int ROOT_CA = 644;
                static constexpr int CSR_FILE = 600;
                static constexpr int LOG_FILE = 600;
                static constexpr int CONFIG_FILE = 644;
                static constexpr int RUNTIME_CONFIG_FILE = 644;
                static constexpr int JOB_HANDLER = 700;
            };

            struct PlainConfig : public LoadableFromJsonAndCli
            {
                bool LoadFromJson(const Crt::JsonView &json) override;
                bool LoadFromCliArgs(const CliArgs &cliArgs) override;
                bool Validate() const override;

                static constexpr char CLI_ENDPOINT[] = "--endpoint";
                static constexpr char CLI_CERT[] = "--cert";
                static constexpr char CLI_KEY[] = "--key";
                static constexpr char CLI_ROOT_CA[] = "--root-ca";
                static constexpr char CLI_THING_NAME[] = "--thing-name";

                static constexpr char JSON_KEY_ENDPOINT[] = "endpoint";
                static constexpr char JSON_KEY_CERT[] = "cert";
                static constexpr char JSON_KEY_KEY[] = "key";
                static constexpr char JSON_KEY_ROOT_CA[] = "root-ca";
                static constexpr char JSON_KEY_THING_NAME[] = "thing-name";
                static constexpr char JSON_KEY_JOBS[] = "jobs";
                static constexpr char JSON_KEY_TUNNELING[] = "tunneling";
                static constexpr char JSON_KEY_DEVICE_DEFENDER[] = "device-defender";
                static constexpr char JSON_KEY_FLEET_PROVISIONING[] = "fleet-provisioning";
                static constexpr char JSON_KEY_RUNTIME_CONFIG[] = "runtime-config";
                static constexpr char JSON_KEY_LOGGING[] = "logging";

                Aws::Crt::Optional<std::string> endpoint;
                Aws::Crt::Optional<std::string> cert;
                Aws::Crt::Optional<std::string> key;
                Aws::Crt::Optional<std::string> rootCa;
                Aws::Crt::Optional<std::string> thingName;

                struct LogConfig : public LoadableFromJsonAndCli
                {
                    bool LoadFromJson(const Crt::JsonView &json) override;
                    bool LoadFromCliArgs(const CliArgs &cliArgs) override;
                    bool Validate() const override;
                    int ParseLogLevel(std::string value);
                    std::string ParseLogType(std::string value);

                    static constexpr char LOG_TYPE_FILE[] = "file";
                    static constexpr char LOG_TYPE_STDOUT[] = "stdout";

                    static constexpr char CLI_LOG_LEVEL[] = "--log-level";
                    static constexpr char CLI_LOG_TYPE[] = "--log-type";
                    static constexpr char CLI_LOG_FILE[] = "--log-file";

                    static constexpr char JSON_KEY_LOG_LEVEL[] = "level";
                    static constexpr char JSON_KEY_LOG_TYPE[] = "type";
                    static constexpr char JSON_KEY_LOG_FILE[] = "file";

                    int logLevel{3};
                    std::string type;
                    std::string file;
                };
                LogConfig logConfig;

                struct Jobs : public LoadableFromJsonAndCli
                {
                    bool LoadFromJson(const Crt::JsonView &json) override;
                    bool LoadFromCliArgs(const CliArgs &cliArgs) override;
                    bool Validate() const override;

                    static constexpr char CLI_ENABLE_JOBS[] = "--enable-jobs";
                    static constexpr char CLI_HANDLER_DIR[] = "--jobs-handler-dir";
                    static constexpr char JSON_KEY_ENABLED[] = "enabled";
                    static constexpr char JSON_KEY_HANDLER_DIR[] = "handler-directory";

                    bool enabled{false};
                    std::string handlerDir;
                };
                Jobs jobs;

                struct Tunneling : public LoadableFromJsonAndCli
                {
                    bool LoadFromJson(const Crt::JsonView &json) override;
                    bool LoadFromCliArgs(const CliArgs &cliArgs) override;
                    bool Validate() const override;

                    static constexpr char CLI_ENABLE_TUNNELING[] = "--enable-tunneling";
                    static constexpr char CLI_TUNNELING_DISABLE_NOTIFICATION[] = "--tunneling-disable-notification";
                    static constexpr char CLI_TUNNELING_DESTINATION_ACCESS_TOKEN[] =
                        "--tunneling-destination-access-token";
                    static constexpr char CLI_TUNNELING_REGION[] = "--tunneling-region";
                    static constexpr char CLI_TUNNELING_SERVICE[] = "--tunneling-service";
                    static constexpr char JSON_KEY_ENABLED[] = "enabled";
                    static constexpr char JSON_KEY_ENDPOINT[] = "endpoint";

                    bool enabled{false};
                    bool subscribeNotification{true};
                    Aws::Crt::Optional<std::string> destinationAccessToken;
                    Aws::Crt::Optional<std::string> region;
                    Aws::Crt::Optional<int> port;

                    // Normally the endpoint is determined by `region` only. This is only used to override the normal
                    // endpoint such as when testing against the gamma stage.
                    Aws::Crt::Optional<std::string> endpoint;
                };
                Tunneling tunneling;

                struct DeviceDefender : public LoadableFromJsonAndCli
                {
                    bool LoadFromJson(const Crt::JsonView &json) override;
                    bool LoadFromCliArgs(const CliArgs &cliArgs) override;
                    bool Validate() const override;

                    static constexpr char CLI_ENABLE_DEVICE_DEFENDER[] = "--enable-device-defender";
                    static constexpr char CLI_DEVICE_DEFENDER_INTERVAL[] = "--device-defender-interval";

                    static constexpr char JSON_KEY_ENABLED[] = "enabled";
                    static constexpr char JSON_KEY_INTERVAL[] = "interval-in-seconds";

                    bool enabled{false};
                    Aws::Crt::Optional<int> interval;
                };
                DeviceDefender deviceDefender;

                struct FleetProvisioning : public LoadableFromJsonAndCli
                {
                    bool LoadFromJson(const Crt::JsonView &json) override;
                    bool LoadFromCliArgs(const CliArgs &cliArgs) override;
                    bool Validate() const override;

                    static constexpr char CLI_ENABLE_FLEET_PROVISIONING[] = "--enable-fleet-provisioning";
                    static constexpr char CLI_FLEET_PROVISIONING_TEMPLATE_NAME[] = "--fleet-provisioning-template-name";
                    static constexpr char CLI_FLEET_PROVISIONING_CSR_FILE[] = "--csr-file";

                    static constexpr char JSON_KEY_ENABLED[] = "enabled";
                    static constexpr char JSON_KEY_TEMPLATE_NAME[] = "template-name";
                    static constexpr char JSON_KEY_CSR_FILE[] = "csr-file";

                    bool enabled{false};
                    Aws::Crt::Optional<std::string> templateName;
                    Aws::Crt::Optional<std::string> csrFile;
                };
                FleetProvisioning fleetProvisioning;

                struct FleetProvisioningRuntimeConfig : public LoadableFromJsonAndCli
                {
                    bool LoadFromJson(const Crt::JsonView &json) override;
                    bool LoadFromCliArgs(const CliArgs &cliArgs) override;
                    bool Validate() const override;

                    static constexpr char JSON_KEY_COMPLETED_FLEET_PROVISIONING[] = "completed-fp";
                    static constexpr char JSON_KEY_CERT[] = "cert";
                    static constexpr char JSON_KEY_KEY[] = "key";
                    static constexpr char JSON_KEY_THING_NAME[] = "thing-name";

                    bool completedFleetProvisioning{false};
                    Aws::Crt::Optional<std::string> cert;
                    Aws::Crt::Optional<std::string> key;
                    Aws::Crt::Optional<std::string> thingName;
                };
                FleetProvisioningRuntimeConfig fleetProvisioningRuntimeConfig;
            };

            class Config
            {
              public:
                Config() = default;
                ~Config() = default;

                static constexpr char TAG[] = "Config.cpp";
                static constexpr char DEFAULT_CONFIG_DIR[] = "~/.aws-iot-device-client/";
                static constexpr char DEFAULT_KEY_DIR[] = "~/.aws-iot-device-client/keys/";
                static constexpr char DEFAULT_CONFIG_FILE[] = "~/.aws-iot-device-client/aws-iot-device-client.conf";
                static constexpr char DEFAULT_FLEET_PROVISIONING_RUNTIME_CONFIG_FILE[] =
                    "~/.aws-iot-device-client/aws-iot-device-client-runtime.conf";

                static constexpr char CLI_HELP[] = "--help";
                static constexpr char CLI_EXPORT_DEFAULT_SETTINGS[] = "--export-default-settings";
                static constexpr char CLI_CONFIG_FILE[] = "--config-file";

                static bool ParseCliArgs(int argc, char *argv[], CliArgs &cliArgs);
                bool ValidateAndStoreRuntimeConfig();
                bool ParseConfigFile(const std::string &file, bool isRuntimeConfig);
                bool init(const CliArgs &cliArgs);

                PlainConfig config;

              private:
                static void PrintHelpMessage();
                static bool ExportDefaultSetting(const std::string &file);
            };
        } // namespace DeviceClient
    }     // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_CONFIG_H
