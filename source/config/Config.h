// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_CONFIG_H
#define DEVICE_CLIENT_CONFIG_H

#include <aws/crt/Api.h>
#include <aws/crt/JsonObject.h>
#include <aws/crt/Optional.h>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <map>
#include <vector>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            using CliArgs = std::map<std::string, std::string>;
            static constexpr char DC_FATAL_ERROR[] = "AWS IOT DEVICE CLIENT FATAL ERROR";

            class LoadableFromJsonAndCliAndEnvironment
            {
              public:
                virtual ~LoadableFromJsonAndCliAndEnvironment() = default;
                virtual bool LoadFromJson(const Crt::JsonView &json) = 0;
                virtual bool LoadFromCliArgs(const CliArgs &cliArgs) = 0;
                virtual bool LoadFromEnvironment() = 0;
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
                static constexpr int PUBSUB_DIR = 745;
                static constexpr int PKCS11_LIB_DIR = 700;
                static constexpr int SENSOR_PUBLISH_ADDR_DIR = 700;

                /** Files **/
                static constexpr int PRIVATE_KEY = 600;
                static constexpr int PUBLIC_CERT = 644;
                static constexpr int ROOT_CA = 644;
                static constexpr int CSR_FILE = 600;
                static constexpr int LOG_FILE = 600;
                static constexpr int CONFIG_FILE = 640;
                static constexpr int RUNTIME_CONFIG_FILE = 640;
                static constexpr int JOB_HANDLER = 700;
                static constexpr int PUB_SUB_FILES = 600;
                static constexpr int SAMPLE_SHADOW_FILES = 600;
                static constexpr int SENSOR_PUBLISH_ADDR_FILE = 660;
                static constexpr int PKCS11_LIB_FILE = 640;
                static constexpr int HTTP_PROXY_CONFIG_FILE = 600;
            };

            struct PlainConfig : public LoadableFromJsonAndCliAndEnvironment
            {
                bool LoadFromJson(const Crt::JsonView &json) override;
                bool LoadFromCliArgs(const CliArgs &cliArgs) override;
                bool LoadFromEnvironment() override;
                bool Validate() const override;
                /** Serialize configurations To Json Object **/
                void SerializeToObject(Crt::JsonObject &object) const;

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

                static constexpr char JSON_KEY_SAMPLES[] = "samples";
                static constexpr char JSON_KEY_PUB_SUB[] = "pub-sub";

                static constexpr char JSON_KEY_SAMPLE_SHADOW[] = "sample-shadow";
                static constexpr char JSON_KEY_CONFIG_SHADOW[] = "config-shadow";
                static constexpr char JSON_KEY_SENSOR_PUBLISH[] = "sensor-publish";

                static constexpr char DEFAULT_LOCK_FILE_PATH[] = "/run/lock/";

                static constexpr char JSON_KEY_SECURE_ELEMENT[] = "secure-element";

                Aws::Crt::Optional<std::string> endpoint;
                Aws::Crt::Optional<std::string> cert;
                Aws::Crt::Optional<std::string> key;
                Aws::Crt::Optional<std::string> rootCa;
                Aws::Crt::Optional<std::string> thingName;

                std::string lockFilePath{DEFAULT_LOCK_FILE_PATH};

                struct LogConfig : public LoadableFromJsonAndCliAndEnvironment
                {
                    bool LoadFromJson(const Crt::JsonView &json) override;
                    bool LoadFromCliArgs(const CliArgs &cliArgs) override;
                    bool LoadFromEnvironment() override { return true; }
                    bool Validate() const override;
                    int ParseDeviceClientLogLevel(const std::string &value) const;
                    Aws::Crt::LogLevel ParseSDKLogLevel(const std::string &value) const;
                    std::string ParseDeviceClientLogType(const std::string &value) const;
                    std::string StringifyDeviceClientLogLevel(int level) const;
                    std::string StringifySDKLogLevel(Aws::Crt::LogLevel level) const;
                    /** Serialize logging configurations To Json Object **/
                    void SerializeToObject(Crt::JsonObject &object) const;
                    static constexpr char LOG_TYPE_FILE[] = "file";
                    static constexpr char LOG_TYPE_STDOUT[] = "stdout";

                    static constexpr char CLI_LOG_LEVEL[] = "--log-level";
                    static constexpr char CLI_LOG_TYPE[] = "--log-type";
                    static constexpr char CLI_LOG_FILE[] = "--log-file";

                    static constexpr char JSON_KEY_LOG_LEVEL[] = "level";
                    static constexpr char JSON_KEY_LOG_TYPE[] = "type";
                    static constexpr char JSON_KEY_LOG_FILE[] = "file";

                    static constexpr char CLI_ENABLE_SDK_LOGGING[] = "--enable-sdk-logging";
                    static constexpr char CLI_SDK_LOG_LEVEL[] = "--sdk-log-level";
                    static constexpr char CLI_SDK_LOG_FILE[] = "--sdk-log-file";

                    static constexpr char JSON_KEY_ENABLE_SDK_LOGGING[] = "enable-sdk-logging";
                    static constexpr char JSON_KEY_SDK_LOG_LEVEL[] = "sdk-log-level";
                    static constexpr char JSON_KEY_SDK_LOG_FILE[] = "sdk-log-file";

                    int deviceClientlogLevel{3};
                    std::string deviceClientLogtype{LOG_TYPE_STDOUT};
                    std::string deviceClientLogFile{"/var/log/aws-iot-device-client/aws-iot-device-client.log"};

                    bool sdkLoggingEnabled{false};
                    Aws::Crt::LogLevel sdkLogLevel{Aws::Crt::LogLevel::Trace};
                    std::string sdkLogFile{"/var/log/aws-iot-device-client/sdk.log"};
                };
                LogConfig logConfig;

                struct Jobs : public LoadableFromJsonAndCliAndEnvironment
                {
                    bool LoadFromJson(const Crt::JsonView &json) override;
                    bool LoadFromCliArgs(const CliArgs &cliArgs) override;
                    bool LoadFromEnvironment() override { return true; }
                    bool Validate() const override;
                    /** Serialize Job feature To Json Object **/
                    void SerializeToObject(Crt::JsonObject &object) const;

                    static constexpr char CLI_ENABLE_JOBS[] = "--enable-jobs";
                    static constexpr char CLI_HANDLER_DIR[] = "--jobs-handler-dir";
                    static constexpr char JSON_KEY_ENABLED[] = "enabled";
                    static constexpr char JSON_KEY_HANDLER_DIR[] = "handler-directory";

                    bool enabled{true};
                    std::string handlerDir;
                };
                Jobs jobs;

                struct Tunneling : public LoadableFromJsonAndCliAndEnvironment
                {
                    bool LoadFromJson(const Crt::JsonView &json) override;
                    bool LoadFromCliArgs(const CliArgs &cliArgs) override;
                    bool LoadFromEnvironment() override;
                    bool Validate() const override;
                    /** Serialize Tunneling feature To Json Object **/
                    void SerializeToObject(Crt::JsonObject &object) const;

                    static constexpr char CLI_ENABLE_TUNNELING[] = "--enable-tunneling";
                    static constexpr char CLI_TUNNELING_DISABLE_NOTIFICATION[] = "--tunneling-disable-notification";
                    static constexpr char CLI_TUNNELING_REGION[] = "--tunneling-region";
                    static constexpr char CLI_TUNNELING_SERVICE[] = "--tunneling-service";
                    static constexpr char JSON_KEY_ENABLED[] = "enabled";
                    static constexpr char JSON_KEY_ENDPOINT[] = "endpoint";

                    bool enabled{true};
                    bool subscribeNotification{true};
                    Aws::Crt::Optional<std::string> destinationAccessToken;
                    Aws::Crt::Optional<std::string> region;
                    Aws::Crt::Optional<int> port;

                    // Normally the endpoint is determined by `region` only. This is only used to override the normal
                    // endpoint such as when testing against the gamma stage.
                    Aws::Crt::Optional<std::string> endpoint;
                };
                Tunneling tunneling;

                struct DeviceDefender : public LoadableFromJsonAndCliAndEnvironment
                {
                    bool LoadFromJson(const Crt::JsonView &json) override;
                    bool LoadFromCliArgs(const CliArgs &cliArgs) override;
                    bool LoadFromEnvironment() override { return true; }
                    bool Validate() const override;
                    /** Serialize Device Defender feature To Json Object **/
                    void SerializeToObject(Crt::JsonObject &object) const;

                    static constexpr char CLI_ENABLE_DEVICE_DEFENDER[] = "--enable-device-defender";
                    static constexpr char CLI_DEVICE_DEFENDER_INTERVAL[] = "--device-defender-interval";

                    static constexpr char JSON_KEY_ENABLED[] = "enabled";
                    static constexpr char JSON_KEY_INTERVAL[] = "interval";

                    bool enabled{false};
                    int interval{300};
                };
                DeviceDefender deviceDefender;

                struct FleetProvisioning : public LoadableFromJsonAndCliAndEnvironment
                {
                    bool LoadFromJson(const Crt::JsonView &json) override;
                    bool LoadFromCliArgs(const CliArgs &cliArgs) override;
                    bool LoadFromEnvironment() override { return true; }
                    bool Validate() const override;
                    /** Serialize fleet provisioning configurations To Json Object **/
                    void SerializeToObject(Crt::JsonObject &object) const;

                    static constexpr char CLI_ENABLE_FLEET_PROVISIONING[] = "--enable-fleet-provisioning";
                    static constexpr char CLI_FLEET_PROVISIONING_TEMPLATE_NAME[] = "--fleet-provisioning-template-name";
                    static constexpr char CLI_FLEET_PROVISIONING_TEMPLATE_PARAMETERS[] =
                        "--fleet-provisioning-template-parameters";
                    static constexpr char CLI_FLEET_PROVISIONING_CSR_FILE[] = "--csr-file";
                    static constexpr char CLI_FLEET_PROVISIONING_DEVICE_KEY[] = "--device-key";

                    static constexpr char JSON_KEY_ENABLED[] = "enabled";
                    static constexpr char JSON_KEY_TEMPLATE_NAME[] = "template-name";
                    static constexpr char JSON_KEY_TEMPLATE_PARAMETERS[] = "template-parameters";
                    static constexpr char JSON_KEY_CSR_FILE[] = "csr-file";
                    static constexpr char JSON_KEY_DEVICE_KEY[] = "device-key";

                    bool enabled{false};
                    Aws::Crt::Optional<std::string> templateName;
                    Aws::Crt::Optional<std::string> templateParameters;
                    Aws::Crt::Optional<std::string> csrFile;
                    Aws::Crt::Optional<std::string> deviceKey;
                };
                FleetProvisioning fleetProvisioning;

                struct FleetProvisioningRuntimeConfig : public LoadableFromJsonAndCliAndEnvironment
                {
                    bool LoadFromJson(const Crt::JsonView &json) override;
                    bool LoadFromCliArgs(const CliArgs &cliArgs) override;
                    bool LoadFromEnvironment() override { return true; }
                    bool Validate() const override;
                    /** Serialize fleet provisioning runtime configurations To Json Object **/
                    void SerializeToObject(Crt::JsonObject &object) const;

                    static constexpr char JSON_KEY_COMPLETED_FLEET_PROVISIONING[] = "completed-fp";
                    static constexpr char JSON_KEY_CERT[] = "cert";
                    static constexpr char JSON_KEY_KEY[] = "key";
                    static constexpr char JSON_KEY_THING_NAME[] = "thing-name";
                    static constexpr char JSON_KEY_DEVICE_CONFIG[] = "device-config";

                    bool completedFleetProvisioning{false};
                    Aws::Crt::Optional<std::string> cert;
                    Aws::Crt::Optional<std::string> key;
                    Aws::Crt::Optional<std::string> thingName;
                };
                FleetProvisioningRuntimeConfig fleetProvisioningRuntimeConfig;

                struct HttpProxyConfig : public LoadableFromJsonAndCliAndEnvironment
                {
                    bool LoadFromJson(const Crt::JsonView &json) override;
                    bool LoadFromCliArgs(const CliArgs &cliArgs) override;
                    bool LoadFromEnvironment() override { return true; }
                    bool Validate() const override;

                    static constexpr char CLI_HTTP_PROXY_CONFIG_PATH[] = "--http-proxy-config";

                    static constexpr char JSON_KEY_HTTP_PROXY_ENABLED[] = "http-proxy-enabled";
                    static constexpr char JSON_KEY_HTTP_PROXY_HOST[] = "http-proxy-host";
                    static constexpr char JSON_KEY_HTTP_PROXY_PORT[] = "http-proxy-port";
                    static constexpr char JSON_KEY_HTTP_PROXY_AUTH_METHOD[] = "http-proxy-auth-method";
                    static constexpr char JSON_KEY_HTTP_PROXY_USERNAME[] = "http-proxy-username";
                    static constexpr char JSON_KEY_HTTP_PROXY_PASSWORD[] = "http-proxy-password";

                    bool httpProxyEnabled{false};
                    bool httpProxyAuthEnabled{false};
                    Aws::Crt::Optional<std::string> proxyConfigPath;
                    Aws::Crt::Optional<std::string> proxyHost;
                    Aws::Crt::Optional<int> proxyPort;
                    Aws::Crt::Optional<std::string> proxyAuthMethod;
                    Aws::Crt::Optional<std::string> proxyUsername;
                    Aws::Crt::Optional<std::string> proxyPassword;
                };
                HttpProxyConfig httpProxyConfig;

                struct PubSub : public LoadableFromJsonAndCliAndEnvironment
                {
                    bool LoadFromJson(const Crt::JsonView &json) override;
                    bool LoadFromCliArgs(const CliArgs &cliArgs) override;
                    bool LoadFromEnvironment() override { return true; }
                    bool Validate() const override;
                    /** Serialize PubSub feature To Json Object **/
                    void SerializeToObject(Crt::JsonObject &object) const;

                    static constexpr char CLI_ENABLE_PUB_SUB[] = "--enable-pub-sub";
                    static constexpr char CLI_PUB_SUB_PUBLISH_TOPIC[] = "--publish-topic";
                    static constexpr char CLI_PUB_SUB_PUBLISH_FILE[] = "--publish-file";
                    static constexpr char CLI_PUB_SUB_SUBSCRIBE_TOPIC[] = "--subscribe-topic";
                    static constexpr char CLI_PUB_SUB_SUBSCRIBE_FILE[] = "--subscribe-file";

                    static constexpr char JSON_ENABLE_PUB_SUB[] = "enabled";
                    static constexpr char JSON_PUB_SUB_PUBLISH_TOPIC[] = "publish-topic";
                    static constexpr char JSON_PUB_SUB_PUBLISH_FILE[] = "publish-file";
                    static constexpr char JSON_PUB_SUB_SUBSCRIBE_TOPIC[] = "subscribe-topic";
                    static constexpr char JSON_PUB_SUB_SUBSCRIBE_FILE[] = "subscribe-file";
                    static constexpr char JSON_PUB_SUB_PUBLISH_ON_CHANGE[] = "publish-on-change";

                    bool enabled{false};
                    Aws::Crt::Optional<std::string> publishTopic;
                    Aws::Crt::Optional<std::string> publishFile;
                    Aws::Crt::Optional<std::string> subscribeTopic;
                    Aws::Crt::Optional<std::string> subscribeFile;
                    bool publishOnChange{false};
                };
                PubSub pubSub;

                struct SampleShadow : public LoadableFromJsonAndCliAndEnvironment
                {
                    bool LoadFromJson(const Crt::JsonView &json) override;
                    bool LoadFromCliArgs(const CliArgs &cliArgs) override;
                    bool LoadFromEnvironment() override { return true; }
                    bool Validate() const override;
                    /** Serialize SampleShadow feature To Json Object **/
                    void SerializeToObject(Crt::JsonObject &object) const;
                    bool createShadowOutputFile();

                    static constexpr char CLI_ENABLE_SAMPLE_SHADOW[] = "--enable-sample-shadow";
                    static constexpr char CLI_SAMPLE_SHADOW_NAME[] = "--shadow-name";
                    static constexpr char CLI_SAMPLE_SHADOW_INPUT_FILE[] = "--shadow-input-file";
                    static constexpr char CLI_SAMPLE_SHADOW_OUTPUT_FILE[] = "--shadow-output-file";

                    static constexpr char JSON_ENABLE_SAMPLE_SHADOW[] = "enabled";
                    static constexpr char JSON_SAMPLE_SHADOW_NAME[] = "shadow-name";
                    static constexpr char JSON_SAMPLE_SHADOW_INPUT_FILE[] = "shadow-input-file";
                    static constexpr char JSON_SAMPLE_SHADOW_OUTPUT_FILE[] = "shadow-output-file";

                    static constexpr int MAXIMUM_SHADOW_INPUT_FILE_SIZE = 8 * 1024;

                    bool enabled{false};
                    Aws::Crt::Optional<std::string> shadowName;
                    Aws::Crt::Optional<std::string> shadowInputFile;
                    Aws::Crt::Optional<std::string> shadowOutputFile;
                };
                SampleShadow sampleShadow;

                struct ConfigShadow : public LoadableFromJsonAndCliAndEnvironment
                {
                    bool LoadFromJson(const Crt::JsonView &json) override;
                    bool LoadFromCliArgs(const CliArgs &cliArgs) override;
                    bool LoadFromEnvironment() override { return true; }
                    bool Validate() const override;
                    /** Serialize config shadow configurations To Json Object **/
                    void SerializeToObject(Crt::JsonObject &object) const;

                    static constexpr char CLI_ENABLE_CONFIG_SHADOW[] = "--enable-config-shadow";
                    static constexpr char JSON_ENABLE_CONFIG_SHADOW[] = "enabled";

                    bool enabled{false};
                };
                ConfigShadow configShadow;

                struct SecureElement : public LoadableFromJsonAndCliAndEnvironment
                {
                    bool LoadFromJson(const Crt::JsonView &json) override;
                    bool LoadFromCliArgs(const CliArgs &cliArgs) override;
                    bool LoadFromEnvironment() override { return true; }
                    bool Validate() const override;
                    /** Serialize Secure Element configurations To Json Object **/
                    void SerializeToObject(Crt::JsonObject &object) const;

                    static constexpr char CLI_ENABLE_SECURE_ELEMENT[] = "--enable-secure-element";
                    static constexpr char CLI_PKCS11_LIB[] = "--pkcs11-lib";
                    static constexpr char CLI_SECURE_ELEMENT_PIN[] = "--secure-element-pin";
                    static constexpr char CLI_SECURE_ELEMENT_KEY_LABEL[] = "--secure-element-key-label";
                    static constexpr char CLI_SECURE_ELEMENT_SLOT_ID[] = "--secure-element-slot-id";
                    static constexpr char CLI_SECURE_ELEMENT_TOKEN_LABEL[] = "--secure-element-token-label";

                    static constexpr char JSON_ENABLE_SECURE_ELEMENT[] = "enabled";
                    static constexpr char JSON_PKCS11_LIB[] = "pkcs11-lib";
                    static constexpr char JSON_SECURE_ELEMENT_PIN[] = "secure-element-pin";
                    static constexpr char JSON_SECURE_ELEMENT_KEY_LABEL[] = "secure-element-key-label";
                    static constexpr char JSON_SECURE_ELEMENT_SLOT_ID[] = "secure-element-slot-id";
                    static constexpr char JSON_SECURE_ELEMENT_TOKEN_LABEL[] = "secure-element-token-label";

                    bool enabled{false};
                    Aws::Crt::Optional<std::string> pkcs11Lib;
                    Aws::Crt::Optional<std::string> secureElementPin;
                    Aws::Crt::Optional<std::string> secureElementKeyLabel;
                    Aws::Crt::Optional<uint64_t> secureElementSlotId;
                    Aws::Crt::Optional<std::string> secureElementTokenLabel;
                };
                SecureElement secureElement;

                struct SensorPublish : public LoadableFromJsonAndCliAndEnvironment
                {
                    bool LoadFromJson(const Crt::JsonView &json) override;
                    bool LoadFromCliArgs(const CliArgs &cliArgs) override;
                    bool LoadFromEnvironment() override;
                    bool Validate() const override;
                    /** Serialize sensor publish configurations To Json Object **/
                    void SerializeToObject(Crt::JsonObject &object) const;

                    static constexpr char JSON_SENSORS[] = "sensors";
                    static constexpr char JSON_ENABLED[] = "enabled";
                    static constexpr char JSON_NAME[] = "name";
                    static constexpr char JSON_ADDR[] = "addr";
                    static constexpr char JSON_ADDR_POLL_SEC[] = "addr_poll_sec";
                    static constexpr char JSON_BUFFER_TIME_MS[] = "buffer_time_ms";
                    static constexpr char JSON_BUFFER_SIZE[] = "buffer_size";
                    static constexpr char JSON_BUFFER_CAPACITY[] = "buffer_capacity";
                    static constexpr char JSON_EOM_DELIMITER[] = "eom_delimiter";
                    static constexpr char JSON_MQTT_TOPIC[] = "mqtt_topic";
                    static constexpr char JSON_MQTT_DEAD_LETTER_TOPIC[] = "mqtt_dead_letter_topic";
                    static constexpr char JSON_MQTT_HEARTBEAT_TOPIC[] = "mqtt_heartbeat_topic";
                    static constexpr char JSON_HEARTBEAT_TIME_SEC[] = "heartbeat_time_sec";

                    // MAX_SENSOR_SIZE is the maximum number of sensor entries in a valid configuration.
                    //
                    // Increasing this limit will likely also require a user to increase the 5k limit on
                    // the total size of the configuration.
                    static constexpr std::size_t MAX_SENSOR_SIZE = 10;

                    // BUF_CAPACITY_BYTES is the default number of bytes buffered for a single sensor.
                    // When this limit is reached, we will publish all buffered complete messages.
                    //
                    // This limit is based on AWS IoT message broker message size limit of 128KB as of 2022/02.
                    // https://docs.aws.amazon.com/general/latest/gr/iot-core.html#message-broker-limits
                    static constexpr std::int64_t BUF_CAPACITY_BYTES = 128000;

                    // BUF_CAPACITY_BYTES_MIN is the minimum buffer capacity.
                    // The buffer capacity should be configured to be large enough to hold at least a few
                    // multiples of buffer_size messages.
                    static constexpr std::int64_t BUF_CAPACITY_BYTES_MIN = 1024;

                    bool enabled{false};

                    struct SensorSettings
                    {
                        bool enabled{true};
                        Aws::Crt::Optional<std::string> name;
                        Aws::Crt::Optional<std::string> addr;
                        Aws::Crt::Optional<int64_t> addrPollSec{10};
                        Aws::Crt::Optional<int64_t> bufferTimeMs{0};
                        Aws::Crt::Optional<int64_t> bufferSize{0};
                        Aws::Crt::Optional<int64_t> bufferCapacity{BUF_CAPACITY_BYTES};
                        Aws::Crt::Optional<std::string> eomDelimiter;
                        Aws::Crt::Optional<std::string> mqttTopic;
                        Aws::Crt::Optional<std::string> mqttDeadLetterTopic;
                        Aws::Crt::Optional<std::string> mqttHeartbeatTopic;
                        Aws::Crt::Optional<int64_t> heartbeatTimeSec{300};
                    };
                    // If any setting associated with a sensor is found invalid during validation,
                    // then we will disable only that sensor. In order to do this we must modify
                    // the sensor enabled flag. Since Validate() is const member function, the
                    // settings array must be declared mutable to allow this flag to be changed.
                    mutable std::vector<SensorSettings> settings;
                };
                SensorPublish sensorPublish;
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
                static constexpr char DEFAULT_HTTP_PROXY_CONFIG_FILE[] = "~/.aws-iot-device-client/http-proxy.conf";
                static constexpr char DEFAULT_SAMPLE_SHADOW_OUTPUT_DIR[] = "~/.aws-iot-device-client/sample-shadow/";
                static constexpr char DEFAULT_SAMPLE_SHADOW_DOCUMENT_FILE[] = "default-sample-shadow-document";

                static constexpr char CLI_HELP[] = "--help";
                static constexpr char CLI_VERSION[] = "--version";
                static constexpr char CLI_EXPORT_DEFAULT_SETTINGS[] = "--export-default-settings";
                static constexpr char CLI_CONFIG_FILE[] = "--config-file";

                /**
                 * \brief Enum defining several config file types
                 */
                enum ConfigFileType
                {
                    DEVICE_CLIENT_ESSENTIAL_CONFIG,
                    FLEET_PROVISIONING_RUNTIME_CONFIG,
                    HTTP_PROXY_CONFIG
                };

                static bool CheckTerminalArgs(int argc, char *argv[]);
                static bool ParseCliArgs(int argc, char *argv[], CliArgs &cliArgs);
                bool ValidateAndStoreRuntimeConfig();
                bool ValidateAndStoreHttpProxyConfig() const;
                bool ParseConfigFile(const std::string &file, ConfigFileType configFileType);
                bool init(const CliArgs &cliArgs);

                /**
                 * \brief Maximum accepted size for the config file.
                 */
                static constexpr size_t MAX_CONFIG_SIZE = 5000;

                /**
                 * \brief Separator between directories in path.
                 */
                static constexpr char PATH_DIRECTORY_SEPARATOR = '/';

                /**
                 * \brief Use path expansion to return absolute path to device client default configuration directory.
                 * @return
                 */
                static std::string ExpandDefaultConfigDir(bool removeTrailingSeparator = false);

                PlainConfig config;

              private:
                static void PrintHelpMessage();
                static void PrintVersion();
                static bool ExportDefaultSetting(const std::string &file);
            };
        } // namespace DeviceClient
    }     // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_CONFIG_H
