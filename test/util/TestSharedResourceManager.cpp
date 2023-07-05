// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/SharedCrtResourceManager.h"
#include "../../source/util/FileUtils.h"
#include "gtest/gtest.h"

#include <string>
#include <sys/stat.h>

using namespace std;
using namespace Aws;
using namespace Aws::Crt;
using namespace Aws::Iot;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::Util;

const string certDir = "/tmp/device-client-test";
const string certFilePath = "/tmp/device-client-test/aws-iot-device-client-test-cert";
const string keyFilePath = "/tmp/device-client-test/aws-iot-device-client-test-key";

const string invalidSuffix = "-invalid";
const string badPermissionsSuffix = "-bad-permissions";

const string badPermissionsCertFilePath = certFilePath + badPermissionsSuffix;
const string badPermissionsKeyFilePath = keyFilePath + badPermissionsSuffix;

const string invalidCertFilePath = certFilePath + invalidSuffix;
const string invalidKeyFilePath = keyFilePath + invalidSuffix;

static PlainConfig getConfig(string certPath, string keyPath)
{

    std::string jsonString = "{\"endpoint\": \"endpoint value\",\n"
                             "\"cert\": \"" +
                             certPath + "\",\n\"key\": \"" + keyPath + "\"}";

    JsonObject jsonObject(jsonString.c_str());
    JsonView jsonView = jsonObject.View();

    PlainConfig config;
    config.LoadFromJson(jsonView);

    return config;
}

class SharedCrtResourceManagerWrapper : public SharedCrtResourceManager
{
  public:
    bool locateCredentialsWrapper(const PlainConfig &config) { return locateCredentials(config); }
};

class SharedResourceManagerTest : public ::testing::Test
{
  public:
    SharedResourceManagerTest() = default;

    void SetUp() override
    {
        // Initializing allocator, so we can use CJSON lib from SDK in our unit tests.
        manager.initializeAllocator();

        // SharedCrtResourceManager::locateCredentials will check that cert, key files
        // have valid permissions. Create a temporary file to use as a placeholder for this purpose.
        FileUtils::CreateDirectoryWithPermissions(certDir.c_str(), 0700);
        ofstream certFile(certFilePath, std::fstream::app);
        certFile << "test cert" << endl;

        ofstream keyFile(keyFilePath, std::fstream::app);
        keyFile << "test key" << endl;

        chmod(certFilePath.c_str(), 0644);
        chmod(keyFilePath.c_str(), 0600);

        // Create files with incorrect permissions
        ofstream badPermsCertFile(badPermissionsCertFilePath, std::fstream::app);
        badPermsCertFile << "test cert" << endl;

        ofstream badPermsKeyFile(badPermissionsKeyFilePath, std::fstream::app);
        badPermsKeyFile << "test key" << endl;

        chmod(badPermissionsCertFilePath.c_str(), 0777);
        chmod(badPermissionsKeyFilePath.c_str(), 0777);

        // Ensure invalid files do not exist
        std::remove(invalidCertFilePath.c_str());
        std::remove(invalidKeyFilePath.c_str());
    }

    void TearDown() override
    {
        std::remove(certFilePath.c_str());
        std::remove(keyFilePath.c_str());
        std::remove(certDir.c_str());

        std::remove(badPermissionsCertFilePath.c_str());
        std::remove(badPermissionsKeyFilePath.c_str());
    }

    SharedCrtResourceManagerWrapper manager;
};

TEST_F(SharedResourceManagerTest, locateCredentialsHappy)
{

    PlainConfig config;
    config = getConfig(certFilePath, keyFilePath);

    ASSERT_TRUE(manager.locateCredentialsWrapper(config));
}

TEST_F(SharedResourceManagerTest, badPermissionsCert)
{

    PlainConfig config;
    config = getConfig(badPermissionsCertFilePath, keyFilePath);

    ASSERT_FALSE(manager.locateCredentialsWrapper(config));
}

TEST_F(SharedResourceManagerTest, badPermissionsKey)
{

    PlainConfig config;
    config = getConfig(certFilePath, badPermissionsKeyFilePath);

    ASSERT_FALSE(manager.locateCredentialsWrapper(config));
}

TEST_F(SharedResourceManagerTest, invalidCert)
{

    PlainConfig config;
    config = getConfig(invalidCertFilePath, keyFilePath);

    ASSERT_FALSE(manager.locateCredentialsWrapper(config));
}

TEST_F(SharedResourceManagerTest, invalidKey)
{

    PlainConfig config;
    config = getConfig(invalidCertFilePath, keyFilePath);

    ASSERT_FALSE(manager.locateCredentialsWrapper(config));
}

TEST_F(SharedResourceManagerTest, badPermissionsDirectory)
{

    PlainConfig config;
    config = getConfig(certFilePath, keyFilePath);
    chmod(certDir.c_str(), 0777);

    ASSERT_FALSE(manager.locateCredentialsWrapper(config));
}