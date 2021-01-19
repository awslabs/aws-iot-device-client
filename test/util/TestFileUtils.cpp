// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/util/FileUtils.h"
#include "../../source/util/StringUtils.h"
#include "../../source/util/UniqueString.h"
#include "gtest/gtest.h"

#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;
using namespace Aws::Iot::DeviceClient::Util;

TEST(FileUtils, handlesAbsoluteFilePath)
{
    string parentDir = FileUtils::ExtractParentDirectory("/var/log/aws-iot-device-client.log");
    ASSERT_STREQ("/var/log/", parentDir.c_str());
}

TEST(FileUtils, handlesRelativeFilePath)
{
    string parentDir = FileUtils::ExtractParentDirectory("./out/aws-iot-device-client.log");
    ASSERT_STREQ("./out/", parentDir.c_str());
}

TEST(FileUtils, handlesNoDirectories)
{
    string parentDir = FileUtils::ExtractParentDirectory("aws-iot-device-client.log");
    ASSERT_STREQ("./", parentDir.c_str());
}

TEST(FileUtils, handlesRelativeCWD)
{
    string parentDir = FileUtils::ExtractParentDirectory("./aws-iot-device-client.log");
    ASSERT_STREQ("./", parentDir.c_str());
}

TEST(FileUtils, handlesRelativeParent)
{
    string parentDir = FileUtils::ExtractParentDirectory("../aws-iot-device-client.log");
    ASSERT_STREQ("../", parentDir.c_str());
}

TEST(FileUtils, handlesEmptyPath)
{
    string parentDir = FileUtils::ExtractParentDirectory("");
    ASSERT_STREQ("./", parentDir.c_str());
}

TEST(FileUtils, handlesEmptyPathForStoreValueInFile)
{
    ASSERT_FALSE(FileUtils::StoreValueInFile("", ""));
}

TEST(FileUtils, testStoreValueInFile)
{
    ASSERT_TRUE(FileUtils::StoreValueInFile(
        "This file was created as part of testStoreValueInFile unit test.", "/tmp/testStoreValueInFile.txt"));
}
TEST(FileUtils, handlesRootDir)
{
    string rootDir = FileUtils::ExtractParentDirectory("/");
    ASSERT_STREQ("/", rootDir.c_str());
}

TEST(FileUtils, assertsCorrectFilePermissions)
{
    string filePath = "/tmp/" + UniqueString::GetRandomToken(10);

    ofstream file(filePath, std::fstream::app);
    file << "test message" << endl;

    chmod(filePath.c_str(), S_IRUSR | S_IWUSR);
    int permissions = FileUtils::GetFilePermissions(filePath);
    int expectedPermissions = 600;
    ASSERT_EQ(expectedPermissions, permissions);

    std::remove(filePath.c_str());
}

TEST(FileUtils, assertsMkdirSuccess)
{
    string dirPath = "/tmp/" + UniqueString::GetRandomToken(10) + "/";
    int ret = FileUtils::Mkdirs(dirPath);
    struct stat info;
    ASSERT_EQ(stat(dirPath.c_str(), &info), ret);
    ASSERT_TRUE(info.st_mode & S_IFDIR);

    rmdir(dirPath.c_str());
}

TEST(FileUtils, assertsMkdirSuccessWoEndSlash)
{
    string dirPath = "/tmp/" + UniqueString::GetRandomToken(10);
    int ret = FileUtils::Mkdirs(dirPath);
    struct stat info;
    ASSERT_EQ(stat(dirPath.c_str(), &info), ret);
    ASSERT_TRUE(info.st_mode & S_IFDIR);

    rmdir(dirPath.c_str());
}

TEST(FileUtils, assertsMkdirSuccessJustString)
{
    string dirPath = "test.test.test";
    int ret = FileUtils::Mkdirs(dirPath);
    struct stat info;
    ASSERT_EQ(stat(dirPath.c_str(), &info), ret);
    ASSERT_TRUE(info.st_mode & S_IFDIR);

    rmdir(dirPath.c_str());
}

TEST(FileUtils, assertsMkdirSuccessEmptyDirPathFail)
{
    string dirPath = "";
    int ret = FileUtils::Mkdirs(dirPath);
    ASSERT_EQ(-1, ret);

    rmdir(dirPath.c_str());
}

TEST(FileUtils, assertsMkdirSuccessRelativeFolder)
{
    string dirPath = "relative/test/dir/";
    int ret = FileUtils::Mkdirs(dirPath);
    struct stat info;
    ASSERT_EQ(stat(dirPath.c_str(), &info), ret);
    ASSERT_TRUE(info.st_mode & S_IFDIR);

    rmdir(dirPath.c_str());
}

TEST(FileUtils, assertsCorrectDirectoryPermissions)
{
    string dirPath = "/tmp/" + UniqueString::GetRandomToken(10) + "/";
    FileUtils::Mkdirs(dirPath);

    chmod(dirPath.c_str(), S_IRWXU | S_IRGRP | S_IROTH | S_IXOTH);
    int permissions = FileUtils::GetFilePermissions(dirPath);
    int expectedPermissions = 745;
    ASSERT_EQ(expectedPermissions, permissions);

    rmdir(dirPath.c_str());
}

TEST(FileUtils, getsCorrectFileSize)
{
    string filePath = "/tmp/" + UniqueString::GetRandomToken(10);
    ofstream file(filePath, std::fstream::app);
    file << "test message" << endl;

    size_t bytes = FileUtils::GetFileSize(filePath);
    ASSERT_EQ(13, bytes);

    std::remove(filePath.c_str());
}

TEST(FileUtils, getsCorrectFileSizeForEmptyFile)
{
    string filePath = "/tmp/" + UniqueString::GetRandomToken(10);
    ofstream file(filePath, std::fstream::app);

    size_t bytes = FileUtils::GetFileSize(filePath);
    ASSERT_EQ(0, bytes);

    std::remove(filePath.c_str());
}

TEST(FileUtils, getsCorrectFileSizeForNonExistantFile)
{
    string filePath = "/tmp/" + UniqueString::GetRandomToken(10);

    size_t bytes = FileUtils::GetFileSize(filePath);
    ASSERT_EQ(0, bytes);
}

TEST(FileUtils, canSetupDirectoryAndSetPermissions)
{
    string dirPath = "/tmp/" + UniqueString::GetRandomToken(10) + "/";

    bool didSetup = FileUtils::CreateDirectoryWithPermissions(dirPath.c_str(), S_IRWXU);

    ASSERT_TRUE(didSetup);
    ASSERT_EQ(700, FileUtils::GetFilePermissions(dirPath));

    rmdir(dirPath.c_str());
}

TEST(FileUtils, setupDirectoryGoodResultsOnRepeatedAttempts)
{
    string dirPath = "/tmp/" + UniqueString::GetRandomToken(10) + "/";

    bool didSetup = FileUtils::CreateDirectoryWithPermissions(dirPath.c_str(), S_IRWXU);

    ASSERT_TRUE(didSetup);
    ASSERT_EQ(700, FileUtils::GetFilePermissions(dirPath));

    didSetup = FileUtils::CreateDirectoryWithPermissions(dirPath.c_str(), S_IRWXU);

    ASSERT_TRUE(didSetup);
    ASSERT_EQ(700, FileUtils::GetFilePermissions(dirPath));

    rmdir(dirPath.c_str());
}

TEST(FileUtils, setupDirectoryDetectedSetupFailure)
{
    string dirPath = "/dev/null/" + UniqueString::GetRandomToken(10) + "/";

    bool didSetup = FileUtils::CreateDirectoryWithPermissions(dirPath.c_str(), S_IRWXU);

    ASSERT_FALSE(didSetup);
}
