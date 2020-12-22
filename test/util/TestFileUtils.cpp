#include "../../source/util/FileUtils.h"
#include "../../source/util/StringUtils.h"
#include "../../source/util/UniqueString.h"
#include "gtest/gtest.h"

#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <unistd.h>

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

TEST(FileUtils, handlesRootDir)
{
    string rootDir = FileUtils::extractParentDirectory("/");
    ASSERT_STREQ("/", rootDir.c_str());
}

TEST(FileUtils, assertsCorrectFilePermissions)
{
    string filePath = "/tmp/" + UniqueString::getRandomToken(10);

    ofstream file(filePath, std::fstream::app);
    file << "test message" << endl;

    chmod(filePath.c_str(), S_IRUSR | S_IWUSR);
    int permissions = FileUtils::getFilePermissions(filePath);
    int expectedPermissions = 600;
    ASSERT_EQ(expectedPermissions, permissions);

    std::remove(filePath.c_str());
}

TEST(FileUtils, assertsCorrectDirectoryPermissions)
{
    string dirPath = "/tmp/" + UniqueString::getRandomToken(10) + "/";
    FileUtils::mkdirs(dirPath.c_str());

    chmod(dirPath.c_str(), S_IRWXU | S_IRGRP | S_IROTH | S_IXOTH);
    int permissions = FileUtils::getFilePermissions(dirPath);
    int expectedPermissions = 745;
    ASSERT_EQ(expectedPermissions, permissions);

    rmdir(dirPath.c_str());
}

TEST(FileUtils, getsCorrectFileSize)
{
    string filePath = "/tmp/" + UniqueString::getRandomToken(10);
    ofstream file(filePath, std::fstream::app);
    file << "test message" << endl;

    size_t bytes = FileUtils::getFileSize(filePath);
    ASSERT_EQ(13, bytes);

    std::remove(filePath.c_str());
}

TEST(FileUtils, getsCorrectFileSizeForEmptyFile)
{
    string filePath = "/tmp/" + UniqueString::getRandomToken(10);
    ofstream file(filePath, std::fstream::app);

    size_t bytes = FileUtils::getFileSize(filePath);
    ASSERT_EQ(0, bytes);

    std::remove(filePath.c_str());
}

TEST(FileUtils, getsCorrectFileSizeForNonExistantFile)
{
    string filePath = "/tmp/" + UniqueString::getRandomToken(10);

    size_t bytes = FileUtils::getFileSize(filePath);
    ASSERT_EQ(0, bytes);
}

TEST(FileUtils, canSetupDirectoryAndSetPermissions)
{
    string dirPath = "/tmp/" + UniqueString::getRandomToken(10) + "/";

    bool didSetup = FileUtils::createDirectoryWithPermissions(dirPath.c_str(), S_IRWXU);

    ASSERT_TRUE(didSetup);
    ASSERT_EQ(700, FileUtils::getFilePermissions(dirPath));

    rmdir(dirPath.c_str());
}

TEST(FileUtils, setupDirectoryGoodResultsOnRepeatedAttempts)
{
    string dirPath = "/tmp/" + UniqueString::getRandomToken(10) + "/";

    bool didSetup = FileUtils::createDirectoryWithPermissions(dirPath.c_str(), S_IRWXU);

    ASSERT_TRUE(didSetup);
    ASSERT_EQ(700, FileUtils::getFilePermissions(dirPath));

    didSetup = FileUtils::createDirectoryWithPermissions(dirPath.c_str(), S_IRWXU);

    ASSERT_TRUE(didSetup);
    ASSERT_EQ(700, FileUtils::getFilePermissions(dirPath));

    rmdir(dirPath.c_str());
}

TEST(FileUtils, setupDirectoryDetectedSetupFailure)
{
    string dirPath = "/dev/null/" + UniqueString::getRandomToken(10) + "/";

    bool didSetup = FileUtils::createDirectoryWithPermissions(dirPath.c_str(), S_IRWXU);

    ASSERT_FALSE(didSetup);
}
