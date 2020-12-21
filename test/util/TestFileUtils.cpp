#include "../../source/util/FileUtils.h"
#include "gtest/gtest.h"

using namespace std;
using namespace Aws::Iot::DeviceClient::Util;

TEST(FileUtils, handlesAbsoluteFilePath)
{
    string parentDir = FileUtils::extractParentDirectory("/var/log/aws-iot-device-client.log");
    ASSERT_STREQ("/var/log/", parentDir.c_str());
}

TEST(FileUtils, handlesRelativeFilePath)
{
    string parentDir = FileUtils::extractParentDirectory("./out/aws-iot-device-client.log");
    ASSERT_STREQ("./out/", parentDir.c_str());
}

TEST(FileUtils, handlesNoDirectories)
{
    string parentDir = FileUtils::extractParentDirectory("aws-iot-device-client.log");
    ASSERT_STREQ("", parentDir.c_str());
}

TEST(FileUtils, handlesRelativeCWD)
{
    string parentDir = FileUtils::extractParentDirectory("./aws-iot-device-client.log");
    ASSERT_STREQ("./", parentDir.c_str());
}

TEST(FileUtils, handlesRelativeParent)
{
    string parentDir = FileUtils::extractParentDirectory("../aws-iot-device-client.log");
    ASSERT_STREQ("../", parentDir.c_str());
}

TEST(FileUtils, handlesEmptyPath)
{
    string parentDir = FileUtils::extractParentDirectory("");
    ASSERT_STREQ("", parentDir.c_str());
}

TEST(FileUtils, handlesEmptyPathForStoreValueInFile)
{
    ASSERT_FALSE(FileUtils::StoreValueInFile("", ""));
}

TEST(FileUtils, testStoreValueInFile)
{
    ASSERT_TRUE(FileUtils::StoreValueInFile(
        "This file was created as part of testStoreValueInFile unit test. Make sure you run this test with root "
        "permissions",
        "./testStoreValueInFile.txt"));
}
