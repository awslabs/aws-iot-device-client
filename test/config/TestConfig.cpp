// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/config/Config.h"
#include "gtest/gtest.h"
#include <aws/crt/JsonObject.h>
#include <stdlib.h>

using namespace std;
using namespace Aws::Crt;
using namespace Aws::Iot::DeviceClient;

TEST(Config, AllFeaturesEnabled)
{
    constexpr char jsonString[] = R"(
{
	"endpoint": "endpoint value",
	"cert": "cert",
	"key": "key",
	"root-ca": "root-ca",
	"thing-name": "thing-name value",
    "logging": {
        "level": "debug",
        "type": "file",
        "file": "./aws-iot-device-client.log"
    },
    "jobs": {
        "enabled": true
    },
    "tunneling": {
        "enabled": true
    },
    "device-defender": {
        "enabled": true,
        "interval": 300
    },
    "fleet-provisioning": {
        "enabled": true,
        "template-name": "template-name",
		"csr-file": "csr-file",
		"device-key": "device-key"
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    ASSERT_TRUE(config.Validate());
    ASSERT_STREQ("endpoint value", config.endpoint->c_str());
    ASSERT_STREQ("cert", config.cert->c_str());
    ASSERT_STREQ("key", config.key->c_str());
    ASSERT_STREQ("root-ca", config.rootCa->c_str());
    ASSERT_STREQ("thing-name value", config.thingName->c_str());
    ASSERT_STREQ("file", config.logConfig.deviceClientLogtype.c_str());
    ASSERT_STREQ("./aws-iot-device-client.log", config.logConfig.deviceClientLogFile.c_str());
    ASSERT_EQ(3, config.logConfig.deviceClientlogLevel); // Expect DEBUG log level, which is 3
    ASSERT_TRUE(config.jobs.enabled);
    ASSERT_TRUE(config.tunneling.enabled);
    ASSERT_TRUE(config.deviceDefender.enabled);
    ASSERT_TRUE(config.fleetProvisioning.enabled);
    ASSERT_EQ(300, config.deviceDefender.interval);
    ASSERT_STREQ("template-name", config.fleetProvisioning.templateName->c_str());
    ASSERT_STREQ("csr-file", config.fleetProvisioning.csrFile->c_str());
    ASSERT_STREQ("device-key", config.fleetProvisioning.deviceKey->c_str());
}

TEST(Config, HappyCaseMinimumConfig)
{
    constexpr char jsonString[] = R"(
{
	"endpoint": "endpoint value",
	"cert": "cert",
	"key": "key",
	"root-ca": "root-ca",
	"thing-name": "thing-name value"
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    ASSERT_TRUE(config.Validate());
    ASSERT_STREQ("endpoint value", config.endpoint->c_str());
    ASSERT_STREQ("cert", config.cert->c_str());
    ASSERT_STREQ("key", config.key->c_str());
    ASSERT_STREQ("root-ca", config.rootCa->c_str());
    ASSERT_STREQ("thing-name value", config.thingName->c_str());
    ASSERT_TRUE(config.jobs.enabled);
    ASSERT_TRUE(config.tunneling.enabled);
    ASSERT_TRUE(config.deviceDefender.enabled);
    ASSERT_FALSE(config.fleetProvisioning.enabled);
}

TEST(Config, HappyCaseMinimumCli)
{
    CliArgs cliArgs;
    cliArgs[PlainConfig::CLI_ENDPOINT] = "endpoint value";
    cliArgs[PlainConfig::CLI_CERT] = "cert";
    cliArgs[PlainConfig::CLI_KEY] = "key";
    cliArgs[PlainConfig::CLI_ROOT_CA] = "root-ca";
    cliArgs[PlainConfig::CLI_THING_NAME] = "thing-name value";

    PlainConfig config;
    config.LoadFromCliArgs(cliArgs);

    ASSERT_TRUE(config.Validate());
    ASSERT_STREQ("endpoint value", config.endpoint->c_str());
    ASSERT_STREQ("cert", config.cert->c_str());
    ASSERT_STREQ("key", config.key->c_str());
    ASSERT_STREQ("root-ca", config.rootCa->c_str());
    ASSERT_STREQ("thing-name value", config.thingName->c_str());
    ASSERT_TRUE(config.jobs.enabled);
    ASSERT_TRUE(config.tunneling.enabled);
    ASSERT_TRUE(config.deviceDefender.enabled);
    ASSERT_FALSE(config.fleetProvisioning.enabled);
}

TEST(Config, MissingSomeSettings)
{
    constexpr char jsonString[] = R"(
{
    // endpoint is missing
	"cert": "cert",
	"key": "key",
	"root-ca": "root-ca",
	"thing-name": "thing-name value"
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

#if !defined(DISABLE_MQTT)
    // ST_COMPONENT_MODE does not require any settings besides those for Secure Tunneling
    ASSERT_FALSE(config.Validate());
#else
    ASSERT_TRUE(config.Validate());
#endif
}

TEST(Config, SecureTunnelingMinimumConfig)
{
    constexpr char jsonString[] = R"(
{
	"endpoint": "endpoint value",
	"cert": "cert",
	"key": "key",
	"root-ca": "root-ca",
	"thing-name": "thing-name value",
    "tunneling": {
        "enabled": true
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    ASSERT_TRUE(config.Validate());
    ASSERT_TRUE(config.tunneling.enabled);
    ASSERT_TRUE(config.tunneling.subscribeNotification);
}

TEST(Config, SecureTunnelingCli)
{
    constexpr char jsonString[] = R"(
{
	"endpoint": "endpoint value",
	"cert": "cert",
	"key": "key",
	"root-ca": "root-ca",
	"thing-name": "thing-name value",
    "tunneling": {
        "enabled": true
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    CliArgs cliArgs;
    cliArgs[PlainConfig::Tunneling::CLI_TUNNELING_REGION] = "region value";
    cliArgs[PlainConfig::Tunneling::CLI_TUNNELING_SERVICE] = "SSH";
    cliArgs[PlainConfig::Tunneling::CLI_TUNNELING_DISABLE_NOTIFICATION] = "";

    setenv("AWSIOT_TUNNEL_ACCESS_TOKEN", "destination_access_token_value", 1);

    PlainConfig config;
    config.LoadFromJson(jsonView);
    config.LoadFromCliArgs(cliArgs);
    config.LoadFromEnvironment();

    ASSERT_TRUE(config.Validate());
    ASSERT_TRUE(config.tunneling.enabled);
    ASSERT_STREQ("destination_access_token_value", config.tunneling.destinationAccessToken->c_str());
    ASSERT_STREQ("region value", config.tunneling.region->c_str());
#if !defined(EXCLUDE_ST)
    // Do not test against ST GetPortFromService if ST code is excluded
    ASSERT_EQ(22, config.tunneling.port.value());
#endif
    ASSERT_FALSE(config.tunneling.subscribeNotification);
}

TEST(Config, SecureTunnelingDisableSubscription)
{
    constexpr char jsonString[] = R"(
{
	"endpoint": "endpoint value",
	"cert": "cert",
	"key": "key",
	"root-ca": "root-ca",
	"thing-name": "thing-name value",
    "tunneling": {
        "enabled": true
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();
    CliArgs cliArgs;
    cliArgs[PlainConfig::Tunneling::CLI_TUNNELING_DISABLE_NOTIFICATION] = "";
    cliArgs[PlainConfig::Tunneling::CLI_TUNNELING_REGION] = "region value";
    cliArgs[PlainConfig::Tunneling::CLI_TUNNELING_SERVICE] = "SSH";

    setenv("AWSIOT_TUNNEL_ACCESS_TOKEN", "destination_access_token_value", 1);

    PlainConfig config;
    config.LoadFromJson(jsonView);
    config.LoadFromCliArgs(cliArgs);
    config.LoadFromEnvironment();

    ASSERT_TRUE(config.Validate());
    ASSERT_TRUE(config.tunneling.enabled);
    ASSERT_FALSE(config.tunneling.subscribeNotification);
    ASSERT_STREQ("destination_access_token_value", config.tunneling.destinationAccessToken->c_str());
    ASSERT_STREQ("region value", config.tunneling.region->c_str());
    ASSERT_EQ(22, config.tunneling.port.value());
}

TEST(Config, LoggingConfigurationCLI)
{
    constexpr char jsonString[] = R"(
{
	"endpoint": "endpoint value",
	"cert": "cert",
	"key": "key",
	"root-ca": "root-ca",
	"thing-name": "thing-name value",
    "logging": {
        "level": "DEBUG",
        "type": "STDOUT",
        "file": "old-json-log.log"
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    CliArgs cliArgs;
    cliArgs[PlainConfig::LogConfig::CLI_LOG_LEVEL] = "warn";
    cliArgs[PlainConfig::LogConfig::CLI_LOG_TYPE] = "FILE";
    cliArgs[PlainConfig::LogConfig::CLI_LOG_FILE] = "./client.log";

    PlainConfig config;
    config.LoadFromJson(jsonView);
    config.LoadFromCliArgs(cliArgs);

    ASSERT_EQ(1, config.logConfig.deviceClientlogLevel); // Expect WARN log level, which is 1
    ASSERT_STREQ("file", config.logConfig.deviceClientLogtype.c_str());
    ASSERT_STREQ("./client.log", config.logConfig.deviceClientLogFile.c_str());
}

TEST(Config, SDKLoggingConfigurationCLIDefaults)
{
    CliArgs cliArgs;

    PlainConfig config;
    config.LoadFromCliArgs(cliArgs);

    ASSERT_FALSE(config.logConfig.sdkLoggingEnabled);
    ASSERT_EQ(Aws::Crt::LogLevel::Trace, config.logConfig.sdkLogLevel);
    ASSERT_STREQ(PlainConfig::LogConfig::DEFAULT_SDK_LOG_FILE, config.logConfig.sdkLogFile.c_str());
}

TEST(Config, SDKLoggingConfigurationCLIOverride)
{
    CliArgs cliArgs;
    cliArgs[PlainConfig::LogConfig::CLI_ENABLE_SDK_LOGGING] = "";
    cliArgs[PlainConfig::LogConfig::CLI_SDK_LOG_LEVEL] = "Warn";
    cliArgs[PlainConfig::LogConfig::CLI_SDK_LOG_FILE] = "./sdk.log";

    PlainConfig config;
    config.LoadFromCliArgs(cliArgs);

    ASSERT_TRUE(config.logConfig.sdkLoggingEnabled);
    ASSERT_EQ(Aws::Crt::LogLevel::Warn, config.logConfig.sdkLogLevel);
    ASSERT_STREQ("./sdk.log", config.logConfig.sdkLogFile.c_str());
}

TEST(Config, SDKLoggingConfigurationJsonDefaults)
{
    constexpr char jsonString[] = R"(
{
	"endpoint": "endpoint value",
	"cert": "cert",
	"key": "key",
	"root-ca": "root-ca",
	"thing-name": "thing-name value",
    "logging": {
        "level": "DEBUG",
        "type": "STDOUT",
        "file": "client.log"
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    ASSERT_FALSE(config.logConfig.sdkLoggingEnabled);
    ASSERT_EQ(Aws::Crt::LogLevel::Trace, config.logConfig.sdkLogLevel);
    ASSERT_STREQ(PlainConfig::LogConfig::DEFAULT_SDK_LOG_FILE, config.logConfig.sdkLogFile.c_str());
}

TEST(Config, SDKLoggingConfigurationJson)
{
    constexpr char jsonString[] = R"(
{
	"endpoint": "endpoint value",
	"cert": "cert",
	"key": "key",
	"root-ca": "root-ca",
	"thing-name": "thing-name value",
    "logging": {
        "level": "DEBUG",
        "type": "STDOUT",
        "file": "device-client.log",
        "enable-sdk-logging": true,
        "sdk-log-level": "warn",
        "sdk-log-file": "sdk-log.log"
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    ASSERT_TRUE(config.logConfig.sdkLoggingEnabled);
    ASSERT_EQ(Aws::Crt::LogLevel::Warn, config.logConfig.sdkLogLevel);
    ASSERT_STREQ("sdk-log.log", config.logConfig.sdkLogFile.c_str());

    // Also make sure none of the device client log API settings have been modified
    ASSERT_EQ(3, config.logConfig.deviceClientlogLevel);
    ASSERT_STREQ("stdout", config.logConfig.deviceClientLogtype.c_str());
    ASSERT_STREQ("device-client.log", config.logConfig.deviceClientLogFile.c_str());
}

TEST(Config, FleetProvisioningMinimumConfig)
{
    constexpr char jsonString[] = R"(
{
	"endpoint": "endpoint value",
	"cert": "cert",
	"key": "key",
	"root-ca": "root-ca",
	"thing-name": "thing-name value",
    "fleet-provisioning": {
        "enabled": true,
        "template-name": "template-name"
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    ASSERT_TRUE(config.Validate());
    ASSERT_TRUE(config.fleetProvisioning.enabled);
    ASSERT_STREQ("template-name", config.fleetProvisioning.templateName->c_str());
}

TEST(Config, MissingFleetProvisioningConfig)
{
    constexpr char jsonString[] = R"(
{
	"endpoint": "endpoint value",
	"cert": "cert",
	"key": "key",
	"root-ca": "root-ca",
	"thing-name": "thing-name value"
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    CliArgs cliArgs;
    cliArgs[PlainConfig::FleetProvisioning::CLI_ENABLE_FLEET_PROVISIONING] = "true";

    PlainConfig config;
    config.LoadFromJson(jsonView);

    ASSERT_TRUE(config.Validate());

    config.LoadFromCliArgs(cliArgs);

#if !defined(DISABLE_MQTT)
    // ST_COMPONENT_MODE does not require any settings besides those for Secure Tunneling
    ASSERT_FALSE(config.Validate());
    ASSERT_TRUE(config.fleetProvisioning.enabled);
#else
    ASSERT_TRUE(config.Validate());
    ASSERT_FALSE(config.fleetProvisioning.enabled);
#endif
}

TEST(Config, FleetProvisioningCli)
{
    constexpr char jsonString[] = R"(
{
	"endpoint": "endpoint value",
	"cert": "cert",
	"key": "key",
	"root-ca": "root-ca",
	"thing-name": "thing-name value",
    "fleet-provisioning": {
        "enabled": true,
        "template-name": "template-name",
		"csr-file": "csr-file",
		"device-key": "device-key"
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    CliArgs cliArgs;
    cliArgs[PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_TEMPLATE_NAME] = "cli-template-name";
    cliArgs[PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_CSR_FILE] = "cli-csr-file";
    cliArgs[PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_DEVICE_KEY] = "cli-device-key";

    PlainConfig config;
    config.LoadFromJson(jsonView);
    config.LoadFromCliArgs(cliArgs);

    ASSERT_TRUE(config.Validate());
    ASSERT_TRUE(config.fleetProvisioning.enabled);
    ASSERT_STREQ("cli-template-name", config.fleetProvisioning.templateName->c_str());
    ASSERT_STREQ("cli-csr-file", config.fleetProvisioning.csrFile->c_str());
    ASSERT_STREQ("cli-device-key", config.fleetProvisioning.deviceKey->c_str());
}
