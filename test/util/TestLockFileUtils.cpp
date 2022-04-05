// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/util/LockFileUtils.h"
#include "gtest/gtest.h"

#include <fstream>

using namespace std;
using namespace Aws::Iot::DeviceClient::Util;

static const char FILE_PATH[] = "/var/run/devicecl.lock";

TEST(LockFileUtils, lockProcess)
{
    LockFileUtils::ProcessLock();
    string storedPid;
    ifstream fileIn(FILE_PATH);

    if (fileIn >> storedPid)
    {
        ASSERT_GE(stoi(storedPid), 0);
    }
}

TEST(LockFileUtils, unlockProcess)
{
    int res = LockFileUtils::ProcessUnlock();
    ASSERT_EQ(res, 0);
}
