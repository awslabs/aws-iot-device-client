// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/SharedCrtResourceManager.h"
#include "../../source/config/Config.h"
#include "../../source/util/FileUtils.h"
#include "../../source/util/UniqueString.h"

#include <regex>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include <aws/common/allocator.h>
#include <aws/crt/JsonObject.h>
#include <stdlib.h>
#include <sys/stat.h>

using namespace std;
using namespace Aws::Crt;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::Util;

TEST(SerializeConfigTestFixture, SerializeCompleteConfigTest)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "key": "/tmp/aws-iot-device-client-test-file",
    "thing-name": "thing-name value",
    "logging": {
        "level": "INFO",
        "type": "file",
        "file": "./aws-iot-device-client.log",
        "enable-sdk-logging": false,
        "sdk-log-level": "TRACE",
        "sdk-log-file": "/var/log/aws-iot-device-client/sdk.log"
    },
    "jobs": {
        "enabled": true,
        "handler-directory": "directory"
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
        "template-parameters": "{\"SerialNumber\": \"Device-SN\"}",
        "csr-file": "/tmp/aws-iot-device-client-test-file",
        "device-key": "/tmp/aws-iot-device-client-test-file"
    },
    "runtime-config": {
        "completed-fp": false
     },
    "samples": {
		"pub-sub": {
			"enabled": true,
			"publish-topic": "publish_topic",
			"publish-file": "publish_file",
			"subscribe-topic": "subscribe_topic",
			"subscribe-file": "subscribe_file"
		}
	},
    "config-shadow": {
        "enabled": true
      },
    "sample-shadow": {
        "enabled": true,
        "shadow-name": "shadow-name",
        "shadow-input-file": "shadow_input_file",
        "shadow-output-file": "shadow_output_file"
      },
    "secure-element": {
        "enabled": true,
        "pkcs11-lib": "/tmp/aws-iot-device-client-test-file",
        "secure-element-pin": "0000",
        "secure-element-key-label": "key-label",
        "secure-element-slot-id": 1111,
        "secure-element-token-label": "token-label"
      },
      "sensor-publish": {
        "sensors": [
            {
                "name": "sensor_1",
                "enabled": true,
                "addr": "address_1",
                "addr_poll_sec": 10,
                "buffer_time_ms": 0,
                "buffer_size": 0,
                "buffer_capacity": 128000,
                "eom_delimiter": "delim_1",
                "mqtt_topic": "topic_1",
                "mqtt_dead_letter_topic": "dead_letter_topic_1",
                "mqtt_heartbeat_topic": "heart_beat_topic_1",
                "heartbeat_time_sec": 300
            },
            {
                "name": "sensor_2",
                "enabled": true,
                "addr": "address_2",
                "addr_poll_sec": 1,
                "buffer_time_ms": 1,
                "buffer_size": 1,
                "buffer_capacity": 1,
                "eom_delimiter": "delim_2",
                "mqtt_topic": "topic_2",
                "mqtt_dead_letter_topic": "dead_letter_topic_2",
                "mqtt_heartbeat_topic": "heart_beat_topic_2",
                "heartbeat_time_sec": 10
            }
        ]
    }
})";
    // Initializing allocator, so we can use CJSON lib from SDK in our unit tests.
    SharedCrtResourceManager resourceManager;
    resourceManager.initializeAllocator();
    PlainConfig config;

    JsonObject jsonObject(jsonString);
    config.LoadFromJson(jsonObject.View());
    auto inputJsonString = jsonObject.View().WriteCompact();

    JsonObject serializedConfig;
    config.SerializeToObject(serializedConfig);
    auto serializedJsonString = serializedConfig.View().WriteCompact();

    ASSERT_STREQ(inputJsonString.c_str(), serializedJsonString.c_str());
}

TEST(SerializeConfigTestFixture, SerializeBasicConfigTest)
{
    constexpr char jsonString[] = R"(
{
    "logging": {
        "level": "DEBUG",
        "type": "file",
        "file": "./aws-iot-device-client.log",
        "enable-sdk-logging": false,
        "sdk-log-level": "TRACE",
        "sdk-log-file": "/var/log/aws-iot-device-client/sdk.log"
    },
    "jobs": {
        "enabled": true,
        "handler-directory": ""
    },
    "tunneling": {
        "enabled": true
    },
    "device-defender": {
        "enabled": true,
        "interval": 300
    },
    "fleet-provisioning": {
        "enabled": true
    },
    "runtime-config": {
        "completed-fp": false
     },
    "samples": {
        "pub-sub": {
            "enabled": true
        }
    },
    "config-shadow": {
        "enabled": true
    },
    "sample-shadow": {
        "enabled": true
    },
    "secure-element": {
        "enabled": true
    }
})";
    // Initializing allocator, so we can use CJSON lib from SDK in our unit tests.
    SharedCrtResourceManager resourceManager;
    resourceManager.initializeAllocator();
    PlainConfig config;

    JsonObject jsonObject(jsonString);
    config.LoadFromJson(jsonObject.View());
    auto inputJsonString = jsonObject.View().WriteCompact();

    JsonObject serializedConfig;
    config.SerializeToObject(serializedConfig);
    auto serializedJsonString = serializedConfig.View().WriteCompact();

    ASSERT_STREQ(inputJsonString.c_str(), serializedJsonString.c_str());
}