// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "LockFile.h"
#include "../logging/LoggerFactory.h"

#include <csignal>
#include <stdexcept>
#include <unistd.h>

using namespace std;
using namespace Aws::Iot::DeviceClient::Util;
using namespace Aws::Iot::DeviceClient::Logging;

constexpr char LockFile::TAG[];

LockFile::LockFile(const std::string &filename) : filename(filename)
{
    ifstream fileIn(filename);
    string storedPid;
    if (fileIn && fileIn >> storedPid)
    {
        // sets flag if process exists
        if (!(kill(stoi(storedPid), 0) == -1 && errno == ESRCH))
        {
            string processPath = "/proc/" + storedPid + "/cmdline";
            string cmdline;
            ifstream cmd(processPath.c_str());
            // check if process contains name
            if (cmd && cmd >> cmdline && cmdline.find("aws-iot-device-client") != string::npos)
            {
                LOGM_ERROR(TAG, "Unable to open lockfile", filename.c_str());

                throw runtime_error{"Device Client is already running."};
            }
            cmd.close();
        }
    }
    // remove stale pid file
    remove(filename.c_str());
    fileIn.close();

    FILE *file = fopen(filename.c_str(), "wx");
    if (!file || lockf(fileno(file), F_LOCK, 0))
    {
        LOGM_ERROR(TAG, "Unable to open lockfile", filename.c_str());

        throw runtime_error{"Can not write to lockfile."};
    }

    string pid = to_string(getpid());
    fputs(pid.c_str(), file);
    fclose(file);
}

LockFile::~LockFile()
{
    remove(filename.c_str());
}