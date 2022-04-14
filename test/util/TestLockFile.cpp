// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/util/LockFile.h"
#include "gtest/gtest.h"

#include <unistd.h>
#include <thread>
#include <fstream>

using namespace std;
using namespace Aws::Iot::DeviceClient::Util;

TEST(LockFile, normalCreation)
{
    char buf[FILENAME_MAX];
    string path = getcwd(buf, FILENAME_MAX);
    path += "/devicecl.lock";
    unique_ptr<LockFile> lockFile = unique_ptr<LockFile>(new LockFile{path, "./aws-iot-device-client"});

    ifstream fileIn(path);
    ASSERT_TRUE(fileIn);

    string storedPid;
    if (fileIn >> storedPid)
    {
        ASSERT_STREQ(to_string(getpid()).c_str(), storedPid.c_str());
    }
}

TEST(LockFile, earlyDeletion)
{
    char buf[FILENAME_MAX];
    string path = getcwd(buf, FILENAME_MAX);
    path += "/devicecl.lock";
    unique_ptr<LockFile> lockFile = unique_ptr<LockFile>(new LockFile{path, "test-aws-iot-device-client"});
    lockFile.reset();

    ifstream fileIn(path);
    ASSERT_FALSE(fileIn);
}

TEST(LockFile, multipleFiles)
{
    char buf[FILENAME_MAX];
    string path = getcwd(buf, FILENAME_MAX);
    path += "/devicecl.lock";
    unique_ptr<LockFile> lockFile = unique_ptr<LockFile>(new LockFile{path, "test-aws-iot-device-client"});

    try
    {
        unique_ptr<LockFile> lockFile2 = unique_ptr<LockFile>(new LockFile{path, "test-aws-iot-device-client"});
    }
    catch (exception &e)
    {
        ASSERT_STREQ(e.what(), "Device Client is already running.");
    }
}

TEST(LockFile, multipleFilesWithExtendedPath)
{
    char buf[FILENAME_MAX];
    string path = getcwd(buf, FILENAME_MAX);
    path += "/devicecl.lock";
    unique_ptr<LockFile> lockFile = unique_ptr<LockFile>(new LockFile{path, "test-aws-iot-device-client"});

    try
    {
        unique_ptr<LockFile> lockFile2 = unique_ptr<LockFile>(new LockFile{path, "directory/test-aws-iot-device-client"});
        ASSERT_TRUE(false);
    }
    catch (exception &e)
    {
        ASSERT_STREQ(e.what(), "Device Client is already running.");
    }
}

TEST(LockFile, staleFile)
{
    char buf[FILENAME_MAX];
    string path = getcwd(buf, FILENAME_MAX);
    path += "/devicecl.lock";

    ofstream fileOut(path);
    if (fileOut)
    {
        fileOut << "1234";
    }
    fileOut.close();

    unique_ptr<LockFile> lockFile = unique_ptr<LockFile>(new LockFile{path, "test-aws-iot-device-client"});

    ifstream fileIn(path);
    ASSERT_TRUE(fileIn);

    string storedPid;
    if (fileIn >> storedPid)
    {
        ASSERT_STREQ(to_string(getpid()).c_str(), storedPid.c_str());
    }
}

