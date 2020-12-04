// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/config/Config.h"
#include "gtest/gtest.h"
#include <aws/crt/JsonObject.h>

using namespace std;
using namespace Aws::Crt;
using namespace Aws::Iot::DeviceClient;

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

TEST(Config, SecureTunnelingInvalidPort)
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
        "port": 65536
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    ASSERT_FALSE(config.Validate());
}
