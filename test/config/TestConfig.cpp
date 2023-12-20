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

const string filePath = "/tmp/aws-iot-device-client-test-file";
const string filePathOpenPerms = "/tmp/aws-iot-device-client-perm-test-file";
const string nonStandardDir = "/tmp/aws-iot-device-client-test/";
const string rootCaPath = nonStandardDir + "AmazonRootCA1.pem";
const string invalidFilePath = "/tmp/invalid-file-path";
const string addrPathValid = "/tmp/sensors";
const string addrPathInvalid = "/tmp/sensors-invalid-perms";

class ConfigTestFixture : public ::testing::Test
{
  public:
    ConfigTestFixture() = default;
    string outputPath;
    SharedCrtResourceManager resourceManager;

    void SetUp() override
    {
        // Initializing allocator, so we can use CJSON lib from SDK in our unit tests.
        resourceManager.initializeAllocator();

        // Config::Validate will check that cert, key, and root-ca files exist.
        // Create a temporary file to use as a placeholder for this purpose.
        ofstream file(filePath, std::fstream::app);
        file << "test message" << endl;

        ofstream openPermFile(filePathOpenPerms, std::fstream::app);
        openPermFile << "test message" << endl;
        chmod(filePathOpenPerms.c_str(), 0777);

        FileUtils::CreateDirectoryWithPermissions(nonStandardDir.c_str(), 0700);
        ofstream rootCa(rootCaPath, std::fstream::app);
        chmod(rootCaPath.c_str(), 0644);

        // Ensure invalid-file does not exist
        std::remove(invalidFilePath.c_str());
        mode_t validPerms = S_IRUSR | S_IWUSR | S_IXUSR;
        FileUtils::CreateDirectoryWithPermissions(addrPathValid.c_str(), validPerms);

        mode_t invalidPerms = validPerms | S_IRWXO;
        FileUtils::CreateDirectoryWithPermissions(addrPathInvalid.c_str(), invalidPerms);

        ostringstream outputPathStream;
        outputPathStream << Config::DEFAULT_SAMPLE_SHADOW_OUTPUT_DIR << Config::DEFAULT_SAMPLE_SHADOW_DOCUMENT_FILE;

        outputPath = FileUtils::ExtractExpandedPath(outputPathStream.str().c_str());
    }

    void TearDown() override
    {
        std::remove(rootCaPath.c_str());
        std::remove(nonStandardDir.c_str());
        std::remove(filePathOpenPerms.c_str());
        std::remove(filePath.c_str());
        std::remove(addrPathValid.c_str());
        std::remove(addrPathInvalid.c_str());
    }

    static void AssertDefaultFeaturesEnabled(const PlainConfig &config)
    {
        ASSERT_TRUE(config.jobs.enabled);
        ASSERT_TRUE(config.tunneling.enabled);
        ASSERT_FALSE(config.fleetProvisioning.enabled);
        ASSERT_FALSE(config.deviceDefender.enabled);
        ASSERT_FALSE(config.sampleShadow.enabled);
        ASSERT_FALSE(config.sensorPublish.enabled);
        ASSERT_FALSE(config.pubSub.enabled);
    }
};

/**
 * \brief Return CLI populated with a minimum set of arguments and values.
 * @return CLIArgs
 */
static CliArgs makeMinimumCliArgs()
{
    return CliArgs{
        {PlainConfig::CLI_ENDPOINT, "endpoint value"},
        {PlainConfig::CLI_CERT, filePath},
        {PlainConfig::CLI_KEY, filePath},
        {PlainConfig::CLI_THING_NAME, "thing-name value"},
    };
}

TEST_F(ConfigTestFixture, AllFeaturesEnabled)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "key": "/tmp/aws-iot-device-client-test-file",
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
      },
    "secure-element": {
        "enabled": true,
        "pkcs11-lib": "/tmp/aws-iot-device-client-test-file",
        "secure-element-pin": "0000",
        "secure-element-key-label": "key-label",
        "secure-element-slot-id": 1111,
        "secure-element-token-label": "token-label"
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
    ASSERT_FALSE(config.rootCa.has_value());
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
    ASSERT_TRUE(config.secureElement.enabled);
    ASSERT_STREQ(filePath.c_str(), config.secureElement.pkcs11Lib->c_str());
    ASSERT_STREQ("0000", config.secureElement.secureElementPin->c_str());
    ASSERT_STREQ("key-label", config.secureElement.secureElementKeyLabel->c_str());
    ASSERT_TRUE(config.secureElement.secureElementSlotId.has_value());
    ASSERT_EQ(1111, config.secureElement.secureElementSlotId.value());
    ASSERT_STREQ("token-label", config.secureElement.secureElementTokenLabel->c_str());

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

    JsonObject secureElement;
    config.secureElement.SerializeToObject(secureElement);
    ASSERT_TRUE(secureElement.View().GetBool(config.secureElement.JSON_ENABLE_SECURE_ELEMENT));
    ASSERT_STREQ(filePath.c_str(), secureElement.View().GetString(config.secureElement.JSON_PKCS11_LIB).c_str());
    ASSERT_STREQ("0000", secureElement.View().GetString(config.secureElement.JSON_SECURE_ELEMENT_PIN).c_str());
    ASSERT_STREQ(
        "key-label", secureElement.View().GetString(config.secureElement.JSON_SECURE_ELEMENT_KEY_LABEL).c_str());
    ASSERT_EQ(1111, secureElement.View().GetInteger(config.secureElement.JSON_SECURE_ELEMENT_SLOT_ID));
    ASSERT_STREQ(
        "token-label", secureElement.View().GetString(config.secureElement.JSON_SECURE_ELEMENT_TOKEN_LABEL).c_str());
}

TEST_F(ConfigTestFixture, HappyCaseMinimumConfig)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "key": "/tmp/aws-iot-device-client-test-file",
    "thing-name": "thing-name value"
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    ASSERT_TRUE(config.Validate());
    ASSERT_FALSE(config.rootCa.has_value());
    ASSERT_STREQ("endpoint value", config.endpoint->c_str());
    ASSERT_STREQ(filePath.c_str(), config.cert->c_str());
    ASSERT_STREQ(filePath.c_str(), config.key->c_str());
    ASSERT_STREQ("thing-name value", config.thingName->c_str());
    AssertDefaultFeaturesEnabled(config);
}

TEST_F(ConfigTestFixture, ExtractExpandedPathFailureConfig)
{
    constexpr char badCertCharacter[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file|",
    "key": "/tmp/aws-iot-device-client-test-file",
    "thing-name": "thing-name value"
})";
    constexpr char badKeyCharacter[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "key": "/tmp/aws-iot-device-client-test-file|",
    "thing-name": "thing-name value"
})";
    PlainConfig config;

    JsonObject jsonObjectBadCert(badCertCharacter);
    JsonView jsonViewBadCert = jsonObjectBadCert.View();

    JsonObject jsonObjectBadKey(badKeyCharacter);
    JsonView jsonViewBadKey = jsonObjectBadKey.View();

    ASSERT_THROW(config.LoadFromJson(jsonViewBadCert), wordexp_fail_error);
    ASSERT_THROW(config.LoadFromJson(jsonViewBadKey), wordexp_fail_error);
}

TEST_F(ConfigTestFixture, HappyCaseMinimumCli)
{
    CliArgs cliArgs = makeMinimumCliArgs();

    PlainConfig config;
    config.LoadFromCliArgs(cliArgs);

    ASSERT_TRUE(config.Validate());
    ASSERT_STREQ("endpoint value", config.endpoint->c_str());
    ASSERT_STREQ(filePath.c_str(), config.cert->c_str());
    ASSERT_STREQ(filePath.c_str(), config.key->c_str());
    ASSERT_STREQ("thing-name value", config.thingName->c_str());
    AssertDefaultFeaturesEnabled(config);
}

TEST_F(ConfigTestFixture, ExtractExpandedPathFailureCLI)
{
    CliArgs badCertCharacter = CliArgs{
        {PlainConfig::CLI_ENDPOINT, "endpoint value"},
        {PlainConfig::CLI_CERT, filePath + "|"},
        {PlainConfig::CLI_KEY, filePath},
        {PlainConfig::CLI_THING_NAME, "thing-name value"},
    };

    CliArgs badKeyCharacter = CliArgs{
        {PlainConfig::CLI_ENDPOINT, "endpoint value"},
        {PlainConfig::CLI_CERT, filePath},
        {PlainConfig::CLI_KEY, filePath + "|"},
        {PlainConfig::CLI_THING_NAME, "thing-name value"},
    };

    PlainConfig config;

    ASSERT_THROW(config.LoadFromCliArgs(badCertCharacter), wordexp_fail_error);
    ASSERT_THROW(config.LoadFromCliArgs(badKeyCharacter), wordexp_fail_error);
}

/**
 * Explicitly pass a valid root-ca path via JSON
 * Expect Config.rootCa to equal root-ca path
 */
TEST_F(ConfigTestFixture, HappyCaseExplicitRootCaConfig)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test/AmazonRootCA1.pem",
    "key": "/tmp/aws-iot-device-client-test-file",
    "thing-name": "thing-name value"
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    ASSERT_TRUE(config.Validate());
    ASSERT_STREQ(rootCaPath.c_str(), config.rootCa->c_str());
    ASSERT_STREQ("endpoint value", config.endpoint->c_str());
    ASSERT_STREQ(filePath.c_str(), config.cert->c_str());
    ASSERT_STREQ(filePath.c_str(), config.key->c_str());
    ASSERT_STREQ("thing-name value", config.thingName->c_str());
    AssertDefaultFeaturesEnabled(config);
}

/**
 * Explicitly pass a valid root-ca path via CLI
 * Expect Config.rootCa to equal root-ca path
 */
TEST_F(ConfigTestFixture, HappyCaseExplicitRootCaCli)
{
    CliArgs cliArgs = CliArgs{
        {PlainConfig::CLI_ENDPOINT, "endpoint value"},
        {PlainConfig::CLI_ROOT_CA, rootCaPath},
        {PlainConfig::CLI_CERT, filePath},
        {PlainConfig::CLI_KEY, filePath},
        {PlainConfig::CLI_THING_NAME, "thing-name value"},
    };

    PlainConfig config;
    config.LoadFromCliArgs(cliArgs);

    ASSERT_TRUE(config.Validate());
    ASSERT_STREQ(rootCaPath.c_str(), config.rootCa->c_str());
    ASSERT_STREQ("endpoint value", config.endpoint->c_str());
    ASSERT_STREQ(filePath.c_str(), config.cert->c_str());
    ASSERT_STREQ(filePath.c_str(), config.key->c_str());
    ASSERT_STREQ("thing-name value", config.thingName->c_str());
    AssertDefaultFeaturesEnabled(config);
}

/**
 * Explicitly pass root-ca path via JSON with invalid permissions on parent directory
 * Expect validation to fail
 */
TEST_F(ConfigTestFixture, ExplicitRootCaBadParentPermissionsConfig)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test/AmazonRootCA1.pem",
    "key": "/tmp/aws-iot-device-client-test-file",
    "thing-name": "thing-name value"
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    chmod(nonStandardDir.c_str(), 0777);
    ASSERT_FALSE(config.Validate());
}

/**
 * Explicitly pass root-ca path via CLI with invalid permissions on parent directory
 * Expect validation to fail
 */
TEST_F(ConfigTestFixture, ExplicitRootCaBadParentPermissionsCli)
{
    CliArgs cliArgs = CliArgs{
        {PlainConfig::CLI_ENDPOINT, "endpoint value"},
        {PlainConfig::CLI_ROOT_CA, rootCaPath},
        {PlainConfig::CLI_CERT, filePath},
        {PlainConfig::CLI_KEY, filePath},
        {PlainConfig::CLI_THING_NAME, "thing-name value"},
    };

    PlainConfig config;
    config.LoadFromCliArgs(cliArgs);

    chmod(nonStandardDir.c_str(), 0777);

    ASSERT_FALSE(config.Validate());
}

/**
 * Explicitly pass root-ca path via JSON with invalid permissions on root-ca file
 * Expect validation to fail
 */
TEST_F(ConfigTestFixture, ExplicitRootCaBadPermissionsConfig)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test/AmazonRootCA1.pem",
    "key": "/tmp/aws-iot-device-client-test-file",
    "thing-name": "thing-name value"
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    chmod(rootCaPath.c_str(), 0777);

    ASSERT_FALSE(config.Validate());
}

/**
 * Explicitly pass root-ca path via CLI with invalid permissions on root-ca file
 * Expect validation to fail
 */
TEST_F(ConfigTestFixture, ExplicitRootCaBadPermissionsCli)
{
    CliArgs cliArgs = CliArgs{
        {PlainConfig::CLI_ENDPOINT, "endpoint value"},
        {PlainConfig::CLI_ROOT_CA, rootCaPath},
        {PlainConfig::CLI_CERT, filePath},
        {PlainConfig::CLI_KEY, filePath},
        {PlainConfig::CLI_THING_NAME, "thing-name value"},
    };

    PlainConfig config;
    config.LoadFromCliArgs(cliArgs);

    chmod(rootCaPath.c_str(), 0777);

    ASSERT_FALSE(config.Validate());
}

/**
 * Explicitly pass root-ca path to non-existent file via JSON
 * Expect Config to ignore and not set value
 */
TEST_F(ConfigTestFixture, AllFeaturesEnabledInvalidRootCa)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "key": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/invalid-file-path",
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
    ASSERT_FALSE(config.rootCa.has_value());
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

/**
 * Explicitly pass empty root-ca path via JSON
 * Expect Config to ignore and not set value
 */
TEST_F(ConfigTestFixture, emptyRootCaPathConfig)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "key": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "",
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
    ASSERT_FALSE(config.rootCa.has_value());
    ASSERT_STREQ("thing-name value", config.thingName->c_str());
    AssertDefaultFeaturesEnabled(config);
}

/**
 * Explicitly pass invalid root-ca path via CLI
 * Expect Config to ignore and not set value
 */
TEST_F(ConfigTestFixture, InvalidRootCaPathConfigCli)
{
    CliArgs cliArgs;
    cliArgs[PlainConfig::CLI_ENDPOINT] = "endpoint value";
    cliArgs[PlainConfig::CLI_CERT] = filePath;
    cliArgs[PlainConfig::CLI_KEY] = filePath;
    cliArgs[PlainConfig::CLI_THING_NAME] = "thing-name value";
    cliArgs[PlainConfig::CLI_ROOT_CA] = invalidFilePath;

    PlainConfig config;
    config.LoadFromCliArgs(cliArgs);

    ASSERT_TRUE(config.Validate());
    ASSERT_STREQ("endpoint value", config.endpoint->c_str());
    ASSERT_STREQ(filePath.c_str(), config.cert->c_str());
    ASSERT_STREQ(filePath.c_str(), config.key->c_str());
    ASSERT_FALSE(config.rootCa.has_value());
    ASSERT_STREQ("thing-name value", config.thingName->c_str());
    AssertDefaultFeaturesEnabled(config);
}

/**
 * Explicitly pass invalid root-ca path via JSON
 * Expect Config to ignore and not set value
 */
TEST_F(ConfigTestFixture, InvalidRootCaPathConfig)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "key": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/invalid-file-path",
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
    ASSERT_FALSE(config.rootCa.has_value());
    ASSERT_STREQ("thing-name value", config.thingName->c_str());
    AssertDefaultFeaturesEnabled(config);
}

TEST_F(ConfigTestFixture, MissingSomeSettings)
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

TEST_F(ConfigTestFixture, SecureTunnelingMinimumConfig)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test/AmazonRootCA1.pem",
    "key": "/tmp/aws-iot-device-client-test-file",
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

TEST_F(ConfigTestFixture, SecureTunnelingCli)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test/AmazonRootCA1.pem",
    "key": "/tmp/aws-iot-device-client-test-file",
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

TEST_F(ConfigTestFixture, SecureTunnelingDisableSubscription)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test/AmazonRootCA1.pem",
    "key": "/tmp/aws-iot-device-client-test-file",
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

TEST_F(ConfigTestFixture, LoggingConfigurationCLI)
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

TEST_F(ConfigTestFixture, SDKLoggingConfigurationCLIDefaults)
{
    CliArgs cliArgs;

    PlainConfig config;
    config.LoadFromCliArgs(cliArgs);

    ASSERT_FALSE(config.logConfig.sdkLoggingEnabled);
    ASSERT_EQ(Aws::Crt::LogLevel::Trace, config.logConfig.sdkLogLevel);
}

TEST_F(ConfigTestFixture, SDKLoggingConfigurationCLIOverride)
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

TEST_F(ConfigTestFixture, SDKLoggingConfigurationJsonDefaults)
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
}

TEST_F(ConfigTestFixture, SDKLoggingConfigurationJson)
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
        "type": "FILE",
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
    ASSERT_STREQ("file", config.logConfig.deviceClientLogtype.c_str());
    ASSERT_STREQ("device-client.log", config.logConfig.deviceClientLogFile.c_str());
}

TEST_F(ConfigTestFixture, FleetProvisioningMinimumConfig)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test/AmazonRootCA1.pem",
    "key": "/tmp/aws-iot-device-client-test-file",
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

TEST_F(ConfigTestFixture, MissingFleetProvisioningConfig)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test/AmazonRootCA1.pem",
    "key": "/tmp/aws-iot-device-client-test-file",
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

TEST_F(ConfigTestFixture, FleetProvisioningCli)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test/AmazonRootCA1.pem",
    "key": "/tmp/aws-iot-device-client-test-file",
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
#if !defined(DISABLE_MQTT)
    // ST_COMPONENT_MODE does not require any settings besides those for Secure Tunneling
    ASSERT_TRUE(config.fleetProvisioning.enabled);
    ASSERT_STREQ("cli-template-name", config.fleetProvisioning.templateName->c_str());
    ASSERT_STREQ("{\"SerialNumber\": \"Device-SN\"}", config.fleetProvisioning.templateParameters->c_str());
    ASSERT_STREQ(filePath.c_str(), config.fleetProvisioning.csrFile->c_str());
    ASSERT_STREQ(filePath.c_str(), config.fleetProvisioning.deviceKey->c_str());
#endif
}

TEST_F(ConfigTestFixture, DeviceDefenderCli)
{
    constexpr char jsonString[] = R"(
{
	"endpoint": "endpoint value",
	"cert": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test/AmazonRootCA1.pem",
	"key": "/tmp/aws-iot-device-client-test-file",
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

TEST_F(ConfigTestFixture, PubSubSampleConfig)
{
    string samplesFilePath = "/tmp/" + UniqueString::GetRandomToken(10);
    FileUtils::StoreValueInFile("Test", samplesFilePath);
    chmod(samplesFilePath.c_str(), 0600);
    std::string jsonTemplate = R"(
{
	"endpoint": "endpoint value",
	"cert": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test/AmazonRootCA1.pem",
	"key": "/tmp/aws-iot-device-client-test-file",
	"thing-name": "thing-name value",
	"samples": {
		"pub-sub": {
			"enabled": true,
			"publish-topic": "publish_topic",
			"publish-file": "{samplesFilePath}",
			"subscribe-topic": "subscribe_topic",
			"subscribe-file": "{samplesFilePath}"
		}
	}
})";
    auto pattern = regex{R"(\{samplesFilePath\})"};
    auto jsonString = regex_replace(jsonTemplate, pattern, samplesFilePath);

    JsonObject jsonObject(jsonString.c_str());
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    ASSERT_TRUE(config.Validate());
    ASSERT_TRUE(config.pubSub.enabled);
    ASSERT_STREQ("publish_topic", config.pubSub.publishTopic->c_str());
    ASSERT_STREQ(samplesFilePath.c_str(), config.pubSub.publishFile->c_str());
    ASSERT_STREQ("subscribe_topic", config.pubSub.subscribeTopic->c_str());
    ASSERT_STREQ(samplesFilePath.c_str(), config.pubSub.subscribeFile->c_str());
    remove(samplesFilePath.c_str());
}

TEST_F(ConfigTestFixture, PubSubSampleCli)
{
    string samplesFilePath = "/tmp/" + UniqueString::GetRandomToken(10);
    FileUtils::StoreValueInFile("Test", samplesFilePath);
    chmod(samplesFilePath.c_str(), 0600);

    CliArgs cliArgs = makeMinimumCliArgs();
    cliArgs[PlainConfig::PubSub::CLI_ENABLE_PUB_SUB] = "true";
    cliArgs[PlainConfig::PubSub::CLI_PUB_SUB_PUBLISH_TOPIC] = "publish_topic";
    cliArgs[PlainConfig::PubSub::CLI_PUB_SUB_PUBLISH_FILE] = samplesFilePath;
    cliArgs[PlainConfig::PubSub::CLI_PUB_SUB_SUBSCRIBE_TOPIC] = "subscribe_topic";
    cliArgs[PlainConfig::PubSub::CLI_PUB_SUB_SUBSCRIBE_FILE] = samplesFilePath;

    PlainConfig config;
    config.LoadFromCliArgs(cliArgs);

    ASSERT_TRUE(config.Validate());
#if !defined(DISABLE_MQTT)
    // ST_COMPONENT_MODE does not require any settings besides those for Secure Tunneling
    ASSERT_TRUE(config.pubSub.enabled);
    ASSERT_STREQ("publish_topic", config.pubSub.publishTopic->c_str());
    ASSERT_STREQ(samplesFilePath.c_str(), config.pubSub.publishFile->c_str());
    ASSERT_STREQ("subscribe_topic", config.pubSub.subscribeTopic->c_str());
    ASSERT_STREQ(samplesFilePath.c_str(), config.pubSub.subscribeFile->c_str());
#endif
    remove(samplesFilePath.c_str());
}

#if !defined(DISABLE_MQTT)
// These tests are not applicable if MQTT is disabled.
TEST_F(ConfigTestFixture, SampleShadowCli)
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
    "root-ca": "/tmp/aws-iot-device-client-test/AmazonRootCA1.pem",
	"key": "/tmp/aws-iot-device-client-test-file",
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
#endif

TEST_F(ConfigTestFixture, SensorPublishMinimumConfig)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test/AmazonRootCA1.pem",
    "key": "/tmp/aws-iot-device-client-test-file",
    "thing-name": "thing-name value",
    "sensor-publish": {
        "sensors": [
            {
                "addr": "/tmp/sensors/my-sensor-server",
                "eom_delimiter": "[\r\n]+",
                "mqtt_topic": "my-sensor-data"
            }
        ]
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    ASSERT_TRUE(config.Validate());
    ASSERT_TRUE(config.sensorPublish.enabled);
    ASSERT_EQ(config.sensorPublish.settings.size(), 1);
    const auto &settings = config.sensorPublish.settings[0];
    ASSERT_TRUE(settings.enabled);
    ASSERT_EQ(settings.addr.value(), "/tmp/sensors/my-sensor-server");
    ASSERT_EQ(settings.eomDelimiter.value(), "[\r\n]+");
    ASSERT_EQ(settings.mqttTopic.value(), "my-sensor-data");
}

TEST_F(ConfigTestFixture, SensorPublishMinimumConfigMultipleSensors)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test/AmazonRootCA1.pem",
    "key": "/tmp/aws-iot-device-client-test-file",
    "thing-name": "thing-name value",
    "sensor-publish": {
        "sensors": [
            {
                "enabled": true,
                "addr": "/tmp/sensors/my-sensor-server-01",
                "eom_delimiter": "[\r\n]+",
                "mqtt_topic": "my-sensor-data-01"
            },
            {
                "enabled": true,
                "addr": "/tmp/sensors/my-sensor-server-02",
                "eom_delimiter": "[\r\n]+",
                "mqtt_topic": "my-sensor-data-02"
            },
            {
                "enabled": false,
                "addr": "/tmp/sensors/my-sensor-server-03",
                "eom_delimiter": "[\r\n]+",
                "mqtt_topic": "my-sensor-data-03"
            }
        ]
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    ASSERT_TRUE(config.Validate());
    ASSERT_TRUE(config.sensorPublish.enabled);
    ASSERT_EQ(config.sensorPublish.settings.size(), 3);
    {
        const auto &settings = config.sensorPublish.settings[0];
        ASSERT_TRUE(settings.enabled);
        ASSERT_EQ(settings.addr.value(), "/tmp/sensors/my-sensor-server-01");
        ASSERT_EQ(settings.eomDelimiter.value(), "[\r\n]+");
        ASSERT_EQ(settings.mqttTopic.value(), "my-sensor-data-01");
    }
    {
        const auto &settings = config.sensorPublish.settings[1];
        ASSERT_TRUE(settings.enabled);
        ASSERT_EQ(settings.addr.value(), "/tmp/sensors/my-sensor-server-02");
        ASSERT_EQ(settings.eomDelimiter.value(), "[\r\n]+");
        ASSERT_EQ(settings.mqttTopic.value(), "my-sensor-data-02");
    }
    {
        const auto &settings = config.sensorPublish.settings[2];
        ASSERT_FALSE(settings.enabled);
    }
}

TEST_F(ConfigTestFixture, SensorPublishInvalidConfigAddr)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test/AmazonRootCA1.pem",
    "key": "/tmp/aws-iot-device-client-test-file",
    "thing-name": "thing-name value",
    "sensor-publish": {
        "sensors": [
            {
                "addr": "/tmp/sensors-invalid-perms/my-sensor-server",
                "eom_delimiter": "[\r\n]+",
                "mqtt_topic": "my-sensor-data"
            }
        ]
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

#if defined(EXCLUDE_SENSOR_PUBLISH)
    GTEST_SKIP();
#endif
    ASSERT_FALSE(config.Validate()); // Invalid permissions on addr.
    ASSERT_TRUE(config.sensorPublish.enabled);
    ASSERT_EQ(config.sensorPublish.settings.size(), 1);
    const auto &settings = config.sensorPublish.settings[0];
    ASSERT_FALSE(settings.enabled);
}

TEST_F(ConfigTestFixture, SensorPublishInvalidConfigMqttTopicEmpty)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test/AmazonRootCA1.pem",
    "key": "/tmp/aws-iot-device-client-test-file",
    "thing-name": "thing-name value",
    "sensor-publish": {
        "sensors": [
            {
                "addr": "/tmp/sensors/my-sensor-server",
                "eom_delimiter": "[\r\n]+",
                "mqtt_topic": ""
            }
        ]
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

#if defined(EXCLUDE_SENSOR_PUBLISH)
    GTEST_SKIP();
#endif
    ASSERT_FALSE(config.Validate()); // Empty mqtt_topic.
    ASSERT_TRUE(config.sensorPublish.enabled);
    ASSERT_EQ(config.sensorPublish.settings.size(), 1);
    const auto &settings = config.sensorPublish.settings[0];
    ASSERT_FALSE(settings.enabled);
}

TEST_F(ConfigTestFixture, SensorPublishInvalidConfigEomDelimiter)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test/AmazonRootCA1.pem",
    "key": "/tmp/aws-iot-device-client-test-file",
    "thing-name": "thing-name value",
    "sensor-publish": {
        "sensors": [
            {
                "addr": "/tmp/sensors/my-sensor-server",
                "eom_delimiter": "[\r\n+",
                "mqtt_topic": "my-sensor-data"
            }
        ]
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

#if defined(EXCLUDE_SENSOR_PUBLISH)
    GTEST_SKIP();
#endif
    ASSERT_FALSE(config.Validate()); // Invalid eom_delimiter.
    ASSERT_TRUE(config.sensorPublish.enabled);
    ASSERT_EQ(config.sensorPublish.settings.size(), 1);
    const auto &settings = config.sensorPublish.settings[0];
    ASSERT_FALSE(settings.enabled);
}

TEST_F(ConfigTestFixture, SensorPublishInvalidConfigNegativeIntegers)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test/AmazonRootCA1.pem",
    "key": "/tmp/aws-iot-device-client-test-file",
    "thing-name": "thing-name value",
    "sensor-publish": {
        "sensors": [
            {
                "addr": "/tmp/sensors/my-sensor-server",
                "eom_delimiter": "[\r\n]+",
                "mqtt_topic": "my-sensor-data",
                "addr_poll_sec": -1,
                "buffer_time_ms": -1,
                "buffer_size": -1,
                "heartbeat_time_sec": -1
            }
        ]
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

#if defined(EXCLUDE_SENSOR_PUBLISH)
    GTEST_SKIP();
#endif
    ASSERT_FALSE(config.Validate()); // Invalid integer values.
    ASSERT_TRUE(config.sensorPublish.enabled);
    ASSERT_EQ(config.sensorPublish.settings.size(), 1);
    const auto &settings = config.sensorPublish.settings[0];
    ASSERT_FALSE(settings.enabled);
}

TEST_F(ConfigTestFixture, SensorPublishInvalidConfigBufferCapacityTooSmall)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test/AmazonRootCA1.pem",
    "key": "/tmp/aws-iot-device-client-test-file",
    "thing-name": "thing-name value",
    "sensor-publish": {
        "sensors": [
            {
                "addr": "/tmp/sensors/my-sensor-server",
                "eom_delimiter": "[\r\n]+",
                "mqtt_topic": "my-sensor-data",
                "buffer_capacity": 1
            }
        ]
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

#if defined(EXCLUDE_SENSOR_PUBLISH)
    GTEST_SKIP();
#endif
    ASSERT_FALSE(config.Validate()); // Buffer capacity too small.
    ASSERT_TRUE(config.sensorPublish.enabled);
    ASSERT_EQ(config.sensorPublish.settings.size(), 1);
    const auto &settings = config.sensorPublish.settings[0];
    ASSERT_FALSE(settings.enabled);
}

TEST_F(ConfigTestFixture, SensorPublishDisableFeature)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test/AmazonRootCA1.pem",
    "key": "/tmp/aws-iot-device-client-test-file",
    "thing-name": "thing-name value",
    "sensor-publish": {
        "sensors": [
            {
                "enabled": false,
                "addr": "/tmp/sensors/my-sensor-server",
                "eom_delimiter": "[\r\n]+",
                "mqtt_topic": "my-sensor-data"
            }
        ]
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

#if defined(EXCLUDE_SENSOR_PUBLISH)
    GTEST_SKIP();
#endif
    ASSERT_FALSE(config.Validate()); // All sensors disabled, then disable feature.
    ASSERT_FALSE(config.sensorPublish.enabled);
    ASSERT_EQ(config.sensorPublish.settings.size(), 1);
    const auto &settings = config.sensorPublish.settings[0];
    ASSERT_FALSE(settings.enabled);
}

TEST_F(ConfigTestFixture, SecureElementMinimumConfig)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "thing-name": "thing-name value",
    "secure-element": {
        "enabled": true,
        "pkcs11-lib": "/tmp/aws-iot-device-client-test-file",
        "secure-element-pin": "0000"
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    ASSERT_TRUE(config.Validate());
    ASSERT_TRUE(config.secureElement.enabled);
    ASSERT_STREQ(filePath.c_str(), config.secureElement.pkcs11Lib->c_str());
    ASSERT_STREQ("0000", config.secureElement.secureElementPin->c_str());
}

TEST_F(ConfigTestFixture, SecureElementWithFleetProvisioningEnabled)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "thing-name": "thing-name value",
    "secure-element": {
        "enabled": true,
        "pkcs11-lib": "/tmp/aws-iot-device-client-test-file",
        "secure-element-pin": "0000"
    },
    "fleet-provisioning": {
        "enabled": true,
        "template-name": "template-name",
        "csr-file": "/tmp/aws-iot-device-client-test-file"
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    ASSERT_TRUE(config.Validate());
    ASSERT_TRUE(config.secureElement.enabled);
    ASSERT_FALSE(config.key.has_value());
    ASSERT_STREQ(filePath.c_str(), config.secureElement.pkcs11Lib->c_str());
    ASSERT_STREQ("0000", config.secureElement.secureElementPin->c_str());

    ASSERT_TRUE(config.fleetProvisioning.enabled);
    ASSERT_STREQ("template-name", config.fleetProvisioning.templateName->c_str());
    ASSERT_STREQ(filePath.c_str(), config.fleetProvisioning.csrFile->c_str());
    ASSERT_FALSE(config.fleetProvisioning.deviceKey.has_value());
}

TEST_F(ConfigTestFixture, SecureElementInvalidConfig)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test-file",
    "thing-name": "thing-name value",
    "secure-element": {
        "enabled": true,
        "pkcs11-lib": "/tmp/aws-iot-device-client-test-file"
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    ASSERT_FALSE(config.Validate()); // secure element pin missing
    ASSERT_TRUE(config.secureElement.enabled);
    ASSERT_STREQ(filePath.c_str(), config.secureElement.pkcs11Lib->c_str());
    ASSERT_FALSE(config.secureElement.secureElementPin.has_value());
}

#if !defined(DISABLE_MQTT)
// These tests are not applicable if MQTT is disabled.
TEST_F(ConfigTestFixture, SecureElementDisableFeature)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test/AmazonRootCA1.pem",
    "thing-name": "thing-name value",
    "secure-element": {
        "enabled": false
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    ASSERT_FALSE(config.Validate()); // key value is required if secure element is disabled
    ASSERT_FALSE(config.secureElement.enabled);
    ASSERT_FALSE(config.key.has_value());

    CliArgs cliArgs;
    cliArgs[PlainConfig::CLI_KEY] = "/tmp/aws-iot-device-client-test-file";
    config.LoadFromCliArgs(cliArgs);

    ASSERT_TRUE(config.Validate());
    ASSERT_FALSE(config.secureElement.enabled);
    ASSERT_TRUE(config.key.has_value());
}

TEST_F(ConfigTestFixture, SecureElementCli)
{
    constexpr char jsonString[] = R"(
{
    "endpoint": "endpoint value",
    "cert": "/tmp/aws-iot-device-client-test-file",
    "root-ca": "/tmp/aws-iot-device-client-test/AmazonRootCA1.pem",
    "thing-name": "thing-name value",
    "secure-element": {
        "enabled": false
    }
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    ASSERT_FALSE(config.Validate()); // key value is required if secure element is disabled
    ASSERT_FALSE(config.secureElement.enabled);
    ASSERT_FALSE(config.key.has_value());

    CliArgs cliArgs;
    cliArgs[PlainConfig::SecureElement::CLI_ENABLE_SECURE_ELEMENT] = "true";
    cliArgs[PlainConfig::SecureElement::CLI_PKCS11_LIB] = filePath.c_str();
    cliArgs[PlainConfig::SecureElement::CLI_SECURE_ELEMENT_PIN] = "0000";
    config.LoadFromCliArgs(cliArgs);

    ASSERT_TRUE(config.secureElement.enabled);
    ASSERT_FALSE(config.key.has_value());
    ASSERT_STREQ(filePath.c_str(), config.secureElement.pkcs11Lib->c_str());
    ASSERT_EQ("0000", config.secureElement.secureElementPin.value());
    ASSERT_TRUE(config.Validate());
}
#endif

TEST_F(ConfigTestFixture, HTTPProxyConfigHappy)
{
    constexpr char jsonString[] = R"(
{
  "http-proxy-enabled": true,
  "http-proxy-host": "10.0.0.1",
  "http-proxy-port": "8888",
  "http-proxy-auth-method": "UserNameAndPassword",
  "http-proxy-username": "testUserName",
  "http-proxy-password": "12345"
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig::HttpProxyConfig httpProxyConfig;
    httpProxyConfig.LoadFromJson(jsonView);

    ASSERT_TRUE(httpProxyConfig.httpProxyEnabled);
    ASSERT_STREQ("10.0.0.1", httpProxyConfig.proxyHost->c_str());
    ASSERT_EQ(8888, httpProxyConfig.proxyPort.value());
    ASSERT_TRUE(httpProxyConfig.httpProxyAuthEnabled);
    ASSERT_STREQ("UserNameAndPassword", httpProxyConfig.proxyAuthMethod->c_str());
    ASSERT_STREQ("testUserName", httpProxyConfig.proxyUsername->c_str());
    ASSERT_STREQ("12345", httpProxyConfig.proxyPassword->c_str());
}

TEST_F(ConfigTestFixture, HTTPProxyConfigDisabled)
{
    constexpr char jsonString[] = R"(
{
  "http-proxy-enabled": false,
  "http-proxy-host": "10.0.0.1",
  "http-proxy-port": "8888",
  "http-proxy-auth-method": "UserNameAndPassword",
  "http-proxy-username": "testUserName",
  "http-proxy-password": "12345"
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig::HttpProxyConfig httpProxyConfig;
    httpProxyConfig.LoadFromJson(jsonView);

    ASSERT_FALSE(httpProxyConfig.httpProxyEnabled);
}

TEST_F(ConfigTestFixture, HTTPProxyConfigNoAuth)
{
    constexpr char jsonString[] = R"(
{
  "http-proxy-enabled": true,
  "http-proxy-host": "10.0.0.1",
  "http-proxy-port": "8888",
  "http-proxy-auth-method": "None"
})";
    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainConfig::HttpProxyConfig httpProxyConfig;
    httpProxyConfig.LoadFromJson(jsonView);

    ASSERT_TRUE(httpProxyConfig.httpProxyEnabled);
    ASSERT_STREQ("10.0.0.1", httpProxyConfig.proxyHost->c_str());
    ASSERT_EQ(8888, httpProxyConfig.proxyPort.value());
    ASSERT_FALSE(httpProxyConfig.httpProxyAuthEnabled);
    ASSERT_STREQ("None", httpProxyConfig.proxyAuthMethod->c_str());
}
