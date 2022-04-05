// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <csignal>
#include <fstream>
#include <unistd.h>
#include <iostream>
#include "LockFileUtils.h"

using namespace std;
using namespace Aws::Iot::DeviceClient::Util;

constexpr char LockFileUtils::FILE_NAME[];
constexpr char LockFileUtils::PROCESS_NAME[];

int LockFileUtils::ProcessLock()
{
    bool running = false;
    string storedPid;
    string pid;
    ifstream fileIn;

    pid = to_string(getpid());
    fileIn.open(FILE_NAME);
    if (!fileIn.fail())
    {
        if (fileIn >> storedPid)
        {
            if (!(kill(stoi(storedPid), 0) == -1 && errno == ESRCH))
            {
                string path = "/proc/" + storedPid + "/cmdline";
                string cmdline;
                ifstream cmd;

                cmd.open(path.c_str());
                if (cmd >> cmdline && cmdline.find(PROCESS_NAME) != string::npos)
                {
                    running = true;
                }
                cmd.close();
            }
        }
    }
    fileIn.close();

    if (running)
    {
        cout << "Aborting... Device Client is already running" << endl;
        abort();
    }
    else
    {
        WriteToLockFile(pid);
    }
    return 0;
}

int LockFileUtils::ProcessUnlock()
{
    return remove(FILE_NAME);
}

void LockFileUtils::WriteToLockFile(const std::string &pid)
{
    FILE *lockfile;
    lockfile = fopen(FILE_NAME, "w");
    flockfile(lockfile);
    fputs(pid.c_str(), lockfile);
    funlockfile(lockfile);
    fclose(lockfile);
}

