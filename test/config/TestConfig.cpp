// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/config/Config.h"
#include "gtest/gtest.h"
#include <aws/crt/JsonObject.h>

using namespace std;
using namespace Aws::Crt;
using namespace Aws::Iot::DeviceClient;

TEST(Config, AllFeaturesEnabled)
{
    constexpr char jsonString[] = R"(
{
	"endpoint": "endpoint value",
	"cert": "cert value",
	"key": "key value",
	"root-ca": "root-ca value",
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
        "interval-in-seconds": 300
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    ASSERT_TRUE(config.Validate());
    ASSERT_STREQ("endpoint value", config.endpoint->c_str());
    ASSERT_STREQ("cert value", config.cert->c_str());
    ASSERT_STREQ("key value", config.key->c_str());
    ASSERT_STREQ("root-ca value", config.rootCa->c_str());
    ASSERT_STREQ("thing-name value", config.thingName->c_str());
    ASSERT_STREQ("file", config.logConfig.type.c_str());
    ASSERT_STREQ("./aws-iot-device-client.log", config.logConfig.file.c_str());
    ASSERT_EQ(3, config.logConfig.logLevel); // Expect DEBUG log level, which is 3
    ASSERT_TRUE(config.jobs.enabled);
    ASSERT_TRUE(config.tunneling.enabled);
    ASSERT_TRUE(config.deviceDefender.enabled);
    ASSERT_EQ(300, config.deviceDefender.interval.value());
}

TEST(Config, HappyCaseMinimumConfig)
{
    constexpr char jsonString[] = R"(
{
	"endpoint": "endpoint value",
	"cert": "cert value",
	"key": "key value",
	"root-ca": "root-ca value",
	"thing-name": "thing-name value"
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    ASSERT_TRUE(config.Validate());
    ASSERT_STREQ("endpoint value", config.endpoint->c_str());
    ASSERT_STREQ("cert value", config.cert->c_str());
    ASSERT_STREQ("key value", config.key->c_str());
    ASSERT_STREQ("root-ca value", config.rootCa->c_str());
    ASSERT_STREQ("thing-name value", config.thingName->c_str());
    ASSERT_FALSE(config.jobs.enabled);
    ASSERT_FALSE(config.tunneling.enabled);
    ASSERT_FALSE(config.deviceDefender.enabled);
}

TEST(Config, HappyCaseMinimumCli)
{
    CliArgs cliArgs;
    cliArgs[PlainConfig::CLI_ENDPOINT] = "endpoint value";
    cliArgs[PlainConfig::CLI_CERT] = "cert value";
    cliArgs[PlainConfig::CLI_KEY] = "key value";
    cliArgs[PlainConfig::CLI_ROOT_CA] = "root-ca value";
    cliArgs[PlainConfig::CLI_THING_NAME] = "thing-name value";

    PlainConfig config;
    config.LoadFromCliArgs(cliArgs);

    ASSERT_TRUE(config.Validate());
    ASSERT_STREQ("endpoint value", config.endpoint->c_str());
    ASSERT_STREQ("cert value", config.cert->c_str());
    ASSERT_STREQ("key value", config.key->c_str());
    ASSERT_STREQ("root-ca value", config.rootCa->c_str());
    ASSERT_STREQ("thing-name value", config.thingName->c_str());
    ASSERT_FALSE(config.jobs.enabled);
    ASSERT_FALSE(config.tunneling.enabled);
    ASSERT_FALSE(config.deviceDefender.enabled);
}

TEST(Config, MissingSomeSettings)
{
    constexpr char jsonString[] = R"(
{
    // endpoint is missing
	"cert": "cert value",
	"key": "key value",
	"root-ca": "root-ca value",
	"thing-name": "thing-name value"
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    ASSERT_FALSE(config.Validate());
}

TEST(Config, SecureTunnelingMinimumConfig)
{
    constexpr char jsonString[] = R"(
{
	"endpoint": "endpoint value",
	"cert": "cert value",
	"key": "key value",
	"root-ca": "root-ca value",
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
	"cert": "cert value",
	"key": "key value",
	"root-ca": "root-ca value",
	"thing-name": "thing-name value",
    "tunneling": {
        "enabled": true
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    CliArgs cliArgs;
    cliArgs[PlainConfig::Tunneling::CLI_TUNNELING_DESTINATION_ACCESS_TOKEN] = "destination access token value";
    cliArgs[PlainConfig::Tunneling::CLI_TUNNELING_REGION] = "region value";
    cliArgs[PlainConfig::Tunneling::CLI_TUNNELING_SERVICE] = "SSH";
    cliArgs[PlainConfig::Tunneling::CLI_TUNNELING_DISABLE_NOTIFICATION] = "";

    PlainConfig config;
    config.LoadFromJson(jsonView);
    config.LoadFromCliArgs(cliArgs);

    ASSERT_TRUE(config.Validate());
    ASSERT_TRUE(config.tunneling.enabled);
    ASSERT_STREQ("destination access token value", config.tunneling.destinationAccessToken->c_str());
    ASSERT_STREQ("region value", config.tunneling.region->c_str());
    ASSERT_EQ(22, config.tunneling.port.value());
    ASSERT_FALSE(config.tunneling.subscribeNotification);
}

TEST(Config, SecureTunnelingDisableSubscription)
{
    constexpr char jsonString[] = R"(
{
	"endpoint": "endpoint value",
	"cert": "cert value",
	"key": "key value",
	"root-ca": "root-ca value",
	"thing-name": "thing-name value",
    "tunneling": {
        "enabled": true,
        "subscribe-notification": false,
        "destination-access-token": "destination access token value",
        "region": "region value",
        "port": 65535
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    ASSERT_TRUE(config.Validate());
    ASSERT_TRUE(config.tunneling.enabled);
    ASSERT_FALSE(config.tunneling.subscribeNotification);
    ASSERT_STREQ("destination access token value", config.tunneling.destinationAccessToken->c_str());
    ASSERT_STREQ("region value", config.tunneling.region->c_str());
    ASSERT_EQ(65535, config.tunneling.port.value());
}

TEST(Config, SecureTunnelingPortRange)
{
    // Too small
    const char *jsonString = R"(
{
    "enabled": true,
    "subscribe-notification": false,
    "destination-access-token": "destination access token value",
    "region": "region value",
    "port": 0
})";
    unique_ptr<JsonObject> jsonObject = unique_ptr<JsonObject>(new JsonObject(jsonString));
    JsonView jsonView = jsonObject->View();
    unique_ptr<PlainConfig> config = unique_ptr<PlainConfig>(new PlainConfig());
    config->tunneling.LoadFromJson(jsonView);
    ASSERT_FALSE(config->tunneling.Validate());

    // Negative port
    jsonString = R"(
{
    "enabled": true,
    "subscribe-notification": false,
    "destination-access-token": "destination access token value",
    "region": "region value",
    "port": -1
})";
    jsonObject = unique_ptr<JsonObject>(new JsonObject(jsonString));
    jsonView = jsonObject->View();
    config = unique_ptr<PlainConfig>(new PlainConfig());
    config->tunneling.LoadFromJson(jsonView);
    ASSERT_FALSE(config->tunneling.Validate());

    // Too large
    jsonString = R"(
{
    "enabled": true,
    "subscribe-notification": false,
    "destination-access-token": "destination access token value",
    "region": "region value",
    "port": 65536
})";
    jsonObject = unique_ptr<JsonObject>(new JsonObject(jsonString));
    jsonView = jsonObject->View();
    config = unique_ptr<PlainConfig>(new PlainConfig());
    config->tunneling.LoadFromJson(jsonView);
    ASSERT_FALSE(config->tunneling.Validate());

    // Within range
    jsonString = R"(
{
    "enabled": true,
    "subscribe-notification": false,
    "destination-access-token": "destination access token value",
    "region": "region value",
    "port": 22
})";
    jsonObject = unique_ptr<JsonObject>(new JsonObject(jsonString));
    jsonView = jsonObject->View();
    config = unique_ptr<PlainConfig>(new PlainConfig());
    config->tunneling.LoadFromJson(jsonView);
    ASSERT_TRUE(config->tunneling.Validate());
}

TEST(Config, LoggingConfigurationCLI)
{
    constexpr char jsonString[] = R"(
{
	"endpoint": "endpoint value",
	"cert": "cert value",
	"key": "key value",
	"root-ca": "root-ca value",
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

    ASSERT_EQ(1, config.logConfig.logLevel); // Expect WARN log level, which is 1
    ASSERT_STREQ("file", config.logConfig.type.c_str());
    ASSERT_STREQ("./client.log", config.logConfig.file.c_str());
}
