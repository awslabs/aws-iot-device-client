// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/config/Config.h"
#include "../../source/util/FileUtils.h"
#include "../../source/util/UniqueString.h"
#include "gtest/gtest.h"
#include <aws/crt/JsonObject.h>
#include <stdlib.h>
#include <sys/stat.h>

using namespace std;
using namespace Aws::Crt;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::Util;

const string filePath = "/tmp/aws-iot-device-client-test-file";

TEST(Config, CreateTempFile)
{
    ofstream file(filePath, std::fstream::app);
    file << "test message" << endl;
    ASSERT_TRUE(FileUtils::FileExists(filePath));
}

TEST(Config, AllFeaturesEnabled)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "key": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test-file",
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
        "csr-file": "/tmp/aws-iot-device-client-test-file",
        "device-key": "/tmp/aws-iot-device-client-test-file",
        "template-parameters": "{\"SerialNumber\": \"Device-SN\"}"
    },
    "samples": {
		"pub-sub": {
			"enabled": true,
			"publish-topic": "publish_topic",
			"subscribe-topic": "subscribe_topic"
		}
	},
    "config-shadow": {
        "enabled": true
      },
    "sample-shadow": {
        "enabled": true,
        "shadow-name": "shadow-name",
        "shadow-input-file": "",
        "shadow-output-file": ""
      }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    ASSERT_TRUE(config.Validate());
    ASSERT_STREQ("endpoint value", config.endpoint->c_str());
    ASSERT_STREQ(filePath.c_str(), config.cert->c_str());
    ASSERT_STREQ(filePath.c_str(), config.key->c_str());
    ASSERT_STREQ(filePath.c_str(), config.rootCa->c_str());
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
    ASSERT_STREQ("{\"SerialNumber\": \"Device-SN\"}", config.fleetProvisioning.templateParameters->c_str());
    ASSERT_STREQ(filePath.c_str(), config.fleetProvisioning.csrFile->c_str());
    ASSERT_STREQ(filePath.c_str(), config.fleetProvisioning.deviceKey->c_str());
    ASSERT_TRUE(config.configShadow.enabled);
    ASSERT_TRUE(config.sampleShadow.enabled);
    ASSERT_STREQ("shadow-name", config.sampleShadow.shadowName->c_str());
    ASSERT_FALSE(config.sampleShadow.shadowInputFile.has_value());
    ASSERT_FALSE(config.sampleShadow.shadowOutputFile.has_value());
    ASSERT_TRUE(config.pubSub.enabled);
    ASSERT_STREQ("publish_topic", config.pubSub.publishTopic->c_str());
    ASSERT_STREQ("subscribe_topic", config.pubSub.subscribeTopic->c_str());

    JsonObject tunneling;
    config.tunneling.SerializeToObject(tunneling);
    ASSERT_TRUE(tunneling.View().GetBool(config.tunneling.JSON_KEY_ENABLED));

    JsonObject jobs;
    config.jobs.SerializeToObject(jobs);
    ASSERT_TRUE(jobs.View().GetBool(config.jobs.JSON_KEY_ENABLED));

    JsonObject deviceDefender;
    config.deviceDefender.SerializeToObject(deviceDefender);
    ASSERT_TRUE(deviceDefender.View().GetBool(config.deviceDefender.JSON_KEY_ENABLED));
    ASSERT_EQ(300, deviceDefender.View().GetInteger(config.deviceDefender.JSON_KEY_INTERVAL));

    JsonObject pubsub;
    config.pubSub.SerializeToObject(pubsub);
    ASSERT_TRUE(pubsub.View().GetBool(config.deviceDefender.JSON_KEY_ENABLED));
    ASSERT_STREQ("publish_topic", pubsub.View().GetString(config.pubSub.JSON_PUB_SUB_PUBLISH_TOPIC).c_str());
    ASSERT_STREQ("subscribe_topic", pubsub.View().GetString(config.pubSub.JSON_PUB_SUB_SUBSCRIBE_TOPIC).c_str());

    JsonObject sampleShadow;
    config.sampleShadow.SerializeToObject(sampleShadow);
    ASSERT_STREQ("shadow-name", sampleShadow.View().GetString(config.sampleShadow.JSON_SAMPLE_SHADOW_NAME).c_str());
    ASSERT_STREQ("", sampleShadow.View().GetString(config.sampleShadow.JSON_SAMPLE_SHADOW_INPUT_FILE).c_str());
    ASSERT_STREQ("", sampleShadow.View().GetString(config.sampleShadow.JSON_SAMPLE_SHADOW_OUTPUT_FILE).c_str());
}

TEST(Config, HappyCaseMinimumConfig)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "key": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test-file",
    "thing-name": "thing-name value"
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    ASSERT_TRUE(config.Validate());
    ASSERT_STREQ("endpoint value", config.endpoint->c_str());
    ASSERT_STREQ(filePath.c_str(), config.cert->c_str());
    ASSERT_STREQ(filePath.c_str(), config.key->c_str());
    ASSERT_STREQ(filePath.c_str(), config.rootCa->c_str());
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
    cliArgs[PlainConfig::CLI_CERT] = filePath;
    cliArgs[PlainConfig::CLI_KEY] = filePath;
    cliArgs[PlainConfig::CLI_ROOT_CA] = filePath;
    cliArgs[PlainConfig::CLI_THING_NAME] = "thing-name value";

    PlainConfig config;
    config.LoadFromCliArgs(cliArgs);

    ASSERT_TRUE(config.Validate());
    ASSERT_STREQ("endpoint value", config.endpoint->c_str());
    ASSERT_STREQ(filePath.c_str(), config.cert->c_str());
    ASSERT_STREQ(filePath.c_str(), config.key->c_str());
    ASSERT_STREQ(filePath.c_str(), config.rootCa->c_str());
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
    "cert": "/tmp/aws-iot-device-client-test-file",
    "key": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test-file",
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
    "cert": "/tmp/aws-iot-device-client-test-file",
    "key": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test-file",
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
    "cert": "/tmp/aws-iot-device-client-test-file",
    "key": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test-file",
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
    "cert": "/tmp/aws-iot-device-client-test-file",
    "key": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test-file",
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
    "cert": "/tmp/aws-iot-device-client-test-file",
    "key": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test-file",
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
    "cert": "/tmp/aws-iot-device-client-test-file",
    "key": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test-file",
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
    "cert": "/tmp/aws-iot-device-client-test-file",
    "key": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test-file",
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
    "cert": "/tmp/aws-iot-device-client-test-file",
    "key": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test-file",
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
    "cert": "/tmp/aws-iot-device-client-test-file",
    "key": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test-file",
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
#endif
}

TEST(Config, FleetProvisioningCli)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "key": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test-file",
    "thing-name": "thing-name value",
    "fleet-provisioning": {
        "enabled": true,
        "template-name": "template-name",
        "csr-file": "/tmp/aws-iot-device-client-test-file",
        "device-key": "/tmp/aws-iot-device-client-test-file",
        "template-parameters": "{\"SerialNumber\": \"Device-SN\"}"
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    CliArgs cliArgs;
    cliArgs[PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_TEMPLATE_NAME] = "cli-template-name";
    cliArgs[PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_TEMPLATE_PARAMETERS] =
        "{\"SerialNumber\": \"Device-SN\"}";
    cliArgs[PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_CSR_FILE] = filePath;
    cliArgs[PlainConfig::FleetProvisioning::CLI_FLEET_PROVISIONING_DEVICE_KEY] = filePath;

    PlainConfig config;
    config.LoadFromJson(jsonView);
    config.LoadFromCliArgs(cliArgs);

    ASSERT_TRUE(config.Validate());
    ASSERT_TRUE(config.fleetProvisioning.enabled);
    ASSERT_STREQ("cli-template-name", config.fleetProvisioning.templateName->c_str());
    ASSERT_STREQ("{\"SerialNumber\": \"Device-SN\"}", config.fleetProvisioning.templateParameters->c_str());
    ASSERT_STREQ(filePath.c_str(), config.fleetProvisioning.csrFile->c_str());
    ASSERT_STREQ(filePath.c_str(), config.fleetProvisioning.deviceKey->c_str());
}

TEST(Config, DeviceDefenderCli)
{
    constexpr char jsonString[] = R"(
{
	"endpoint": "endpoint value",
	"cert": "/tmp/aws-iot-device-client-test-file",
	"key": "/tmp/aws-iot-device-client-test-file",
	"root-ca": "/tmp/aws-iot-device-client-test-file",
	"thing-name": "thing-name value",
    "device-defender": {
        "enabled": true,
		"interval": 6
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    CliArgs cliArgs;
    cliArgs[PlainConfig::DeviceDefender::CLI_DEVICE_DEFENDER_INTERVAL] = 6;

    PlainConfig config;
    config.LoadFromJson(jsonView);
    config.LoadFromCliArgs(cliArgs);

    ASSERT_TRUE(config.Validate());
    ASSERT_TRUE(config.deviceDefender.enabled);
    ASSERT_EQ(6, config.deviceDefender.interval);
}

TEST(Config, PubSubSampleCli)
{
    string samplesFilePath = "/tmp/" + UniqueString::GetRandomToken(10);
    FileUtils::StoreValueInFile("Test", samplesFilePath);
    chmod(samplesFilePath.c_str(), 0600);
    constexpr char jsonString[] = R"(
{
	"endpoint": "endpoint value",
	"cert": "/tmp/aws-iot-device-client-test-file",
	"key": "/tmp/aws-iot-device-client-test-file",
	"root-ca": "/tmp/aws-iot-device-client-test-file",
	"thing-name": "thing-name value",
    "samples": {
		"pub-sub": {
			"enabled": true,
			"publish-topic": "publish_topic",
			"publish-file": "/tmp/file",
			"subscribe-topic": "subscribe_topic",
			"subscribe-file": "/tmp/file"
		}
	}
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    CliArgs cliArgs;
    cliArgs[PlainConfig::PubSub::CLI_PUB_SUB_PUBLISH_TOPIC] = "publish_topic";
    cliArgs[PlainConfig::PubSub::CLI_PUB_SUB_PUBLISH_FILE] = samplesFilePath;
    cliArgs[PlainConfig::PubSub::CLI_PUB_SUB_SUBSCRIBE_TOPIC] = "subscribe_topic";
    cliArgs[PlainConfig::PubSub::CLI_PUB_SUB_SUBSCRIBE_FILE] = samplesFilePath;

    PlainConfig config;
    config.LoadFromJson(jsonView);
    config.LoadFromCliArgs(cliArgs);

    ASSERT_TRUE(config.Validate());
    ASSERT_TRUE(config.pubSub.enabled);
    ASSERT_STREQ("publish_topic", config.pubSub.publishTopic->c_str());
    ASSERT_STREQ(samplesFilePath.c_str(), config.pubSub.publishFile->c_str());
    ASSERT_STREQ("subscribe_topic", config.pubSub.subscribeTopic->c_str());
    ASSERT_STREQ(samplesFilePath.c_str(), config.pubSub.subscribeFile->c_str());
    remove(samplesFilePath.c_str());
}

TEST(Config, SampleShadowCli)
{
    string inputFilePath = "/tmp/inputFile";
    FileUtils::StoreValueInFile("Test", inputFilePath);
    chmod(inputFilePath.c_str(), 0600);

    string outputFilePath = "/tmp/inputFile";
    FileUtils::StoreValueInFile("Test", outputFilePath);
    chmod(outputFilePath.c_str(), 0600);
    constexpr char jsonString[] = R"(
{
	"endpoint": "endpoint value",
	"cert": "/tmp/aws-iot-device-client-test-file",
	"key": "/tmp/aws-iot-device-client-test-file",
	"root-ca": "/tmp/aws-iot-device-client-test-file",
	"thing-name": "thing-name value",
    "sample-shadow": {
        "enabled": true,
        "shadow-name": "shadow-name",
        "shadow-input-file": "/tmp/file",
        "shadow-output-file": "/tmp/file"
	}
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    CliArgs cliArgs;
    cliArgs[PlainConfig::SampleShadow::CLI_SAMPLE_SHADOW_NAME] = "shadow-name";
    cliArgs[PlainConfig::SampleShadow::CLI_SAMPLE_SHADOW_INPUT_FILE] = inputFilePath;
    cliArgs[PlainConfig::SampleShadow::CLI_SAMPLE_SHADOW_OUTPUT_FILE] = outputFilePath;

    PlainConfig config;
    config.LoadFromJson(jsonView);
    config.LoadFromCliArgs(cliArgs);

    ASSERT_TRUE(config.Validate());
    ASSERT_TRUE(config.sampleShadow.enabled);
    ASSERT_STREQ("shadow-name", config.sampleShadow.shadowName->c_str());
    ASSERT_STREQ(inputFilePath.c_str(), config.sampleShadow.shadowInputFile->c_str());
    ASSERT_STREQ(outputFilePath.c_str(), config.sampleShadow.shadowOutputFile->c_str());
    remove(inputFilePath.c_str());
    remove(outputFilePath.c_str());
}

TEST(Config, DeleteTempFile)
{
    std::remove(filePath.c_str());
    ASSERT_FALSE(FileUtils::FileExists(filePath));
}