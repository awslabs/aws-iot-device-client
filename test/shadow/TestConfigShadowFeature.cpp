// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/Feature.h"
#include "../../source/shadow/ConfigShadow.h"
#include "gtest/gtest.h"
#include <aws/crt/JsonObject.h>

using namespace std;
using namespace Aws::Crt;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::Util;
using namespace Aws::Iot::DeviceClient::Shadow;
using namespace std;

TEST(ConfigShadowFeature, resetClientConfigWithValidJSON)
{
    constexpr char oldJsonString[] = R"(
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
        "device-key": "device-key",
        "template-parameters": "{\"SerialNumber\": \"Device-SN\"}"
    },
    "samples": {
		"pub-sub": {
			"enabled": true,
			"publish-topic": "publish_topic",
			"subscribe-topic": "subscribe_topic"
		}
	},
    "sample-shadow": {
        "enabled": true,
        "shadow-name": "shadow-name",
        "shadow-input-file": "",
        "shadow-output-file": ""
      }
})";

    SharedCrtResourceManager resourceManager;
    // Initializing allocator, so we can use CJSON lib from SDK in our unit tests.
    resourceManager.initializeAllocator();

    JsonObject oldJsonObject(oldJsonString);
    JsonView jsonView = oldJsonObject.View();
    PlainConfig config;
    config.LoadFromJson(jsonView);

    constexpr char newJsonString[] = R"(
{
    "jobs": {
        "enabled": false
    },
    "tunneling": {
        "enabled": false
    },
    "device-defender": {
        "enabled": true,
        "interval": 200
    },
    "samples": {
		"pub-sub": {
			"enabled": false,
			"publish-topic": "publish_topic",
			"subscribe-topic": "subscribe_topic"
		}
	},
    "sample-shadow": {
        "enabled": false,
        "shadow-name": "shadow-name",
        "shadow-input-file": "",
        "shadow-output-file": ""
    }
})";
    JsonObject jsonObject(newJsonString);
    JsonView newJsonView = jsonObject.View();

    ConfigShadow configShadow;
    configShadow.resetClientConfigWithJSON(config, newJsonView, newJsonView);

    ASSERT_FALSE(config.tunneling.enabled);
    ASSERT_TRUE(config.deviceDefender.enabled);
    ASSERT_EQ(200, config.deviceDefender.interval);
    ASSERT_FALSE(config.pubSub.enabled);
    ASSERT_FALSE(config.sampleShadow.enabled);
    ASSERT_FALSE(config.jobs.enabled);
}

TEST(ConfigShadowFeature, resetClientConfigWithInvalidJSON)
{
    constexpr char oldJsonString[] = R"(
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
        "device-key": "device-key",
        "template-parameters": "{\"SerialNumber\": \"Device-SN\"}"
    },
    "samples": {
		"pub-sub": {
			"enabled": true,
			"publish-topic": "publish_topic",
			"subscribe-topic": "subscribe_topic"
		}
	},
    "sample-shadow": {
        "enabled": false,
        "shadow-name": "shadow-name",
        "shadow-input-file": "",
        "shadow-output-file": ""
      }
})";
    SharedCrtResourceManager resourceManager;
    // Initializing allocator, so we can use CJSON lib from SDK in our unit tests.
    resourceManager.initializeAllocator();

    JsonObject oldJsonObject(oldJsonString);
    JsonView jsonView = oldJsonObject.View();
    PlainConfig config;
    config.LoadFromJson(jsonView);

    constexpr char newJsonString[] = R"(
{
    "jobs": {
        "enabled": false
    },
    "device-defender": {
        "enabled": true,
        "interval": -200
    },

})";
    JsonObject jsonObject(newJsonString);
    JsonView newJsonView = jsonObject.View();

    ConfigShadow configShadow;
    configShadow.resetClientConfigWithJSON(config, newJsonView, newJsonView);

    ASSERT_TRUE(config.jobs.enabled);
    ASSERT_TRUE(config.tunneling.enabled);
    ASSERT_TRUE(config.deviceDefender.enabled);
    ASSERT_EQ(300, config.deviceDefender.interval);
    ASSERT_FALSE(config.sampleShadow.enabled);
}