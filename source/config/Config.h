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

            class LoadableFromJsonAndCli
            {
              public:
                virtual bool LoadFromJson(const Crt::JsonView &json) = 0;
                virtual bool LoadFromCliArgs(const CliArgs &cliArgs) = 0;
                virtual bool Validate() const = 0;
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

                Aws::Crt::Optional<std::string> endpoint;
                Aws::Crt::Optional<std::string> cert;
                Aws::Crt::Optional<std::string> key;
                Aws::Crt::Optional<std::string> rootCa;
                Aws::Crt::Optional<std::string> thingName;

                struct Jobs : public LoadableFromJsonAndCli
                {
                    bool LoadFromJson(const Crt::JsonView &json) override;
                    bool LoadFromCliArgs(const CliArgs &cliArgs) override;
                    bool Validate() const override;

                    static constexpr char CLI_ENABLE_JOBS[] = "--enable-jobs";
                    static constexpr char JSON_KEY_ENABLED[] = "enabled";

                    bool enabled{false};
                };
                Jobs jobs;

                struct Tunneling : public LoadableFromJsonAndCli
                {
                    bool LoadFromJson(const Crt::JsonView &json) override;
                    bool LoadFromCliArgs(const CliArgs &cliArgs) override;
                    bool Validate() const override;

                    static constexpr char CLI_ENABLE_TUNNELING[] = "--enable-tunneling";
                    static constexpr char CLI_TUNNELING_DESTINATION_ACCESS_TOKEN[] =
                        "--tunneling-destination-access-token";
                    static constexpr char CLI_TUNNELING_REGION[] = "--tunneling-region";
                    static constexpr char CLI_TUNNELING_SERVICE[] = "--tunneling-service";
                    static constexpr char CLI_TUNNELING_DISABLE_NOTIFICATION[] = "--tunneling-disable-notification";

                    static constexpr char JSON_KEY_ENABLED[] = "enabled";
                    static constexpr char JSON_KEY_DESTINATION_ACCESS_TOKEN[] = "destination-access-token";
                    static constexpr char JSON_KEY_REGION[] = "region";
                    static constexpr char JSON_KEY_PORT[] = "port";
                    static constexpr char JSON_KEY_SUBSCRIBE_NOTIFICATION[] = "subscribe-notification";

                    bool enabled{false};
                    Aws::Crt::Optional<std::string> destinationAccessToken;
                    Aws::Crt::Optional<std::string> region;
                    Aws::Crt::Optional<int> port;
                    bool subscribeNotification{true};
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
            };

            class Config
            {
              public:
                Config() = default;
                ~Config() = default;

                static constexpr char TAG[] = "Config.cpp";
                static constexpr char DEFAULT_CONFIG_FILE[] = "/etc/aws-iot-device-client.conf";
                static constexpr char JOBS_HANDLER_DIR[] = "handler_directory";

                static constexpr char CLI_HELP[] = "--help";
                static constexpr char CLI_EXPORT_DEFAULT_SETTINGS[] = "--export-default-settings";
                static constexpr char CLI_CONFIG_FILE[] = "--config-file";

                static bool ParseCliArgs(int argc, char *argv[], CliArgs &cliArgs);

                bool init(const CliArgs &cliArgs);

                PlainConfig config;

              private:
                bool ParseConfigFile(const std::string &file);
                static void PrintHelpMessage();
                static void ExportDefaultSetting(const std::string &file);
            };
        } // namespace DeviceClient
    }     // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_CONFIG_H
