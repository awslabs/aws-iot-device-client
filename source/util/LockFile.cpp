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

LockFile::LockFile(const std::string &filename, const std::string &process) : filename(filename)
{
    ifstream fileIn(filename);
    if (fileIn)
    {
        string storedPid;
        if (fileIn >> storedPid && !(kill(stoi(storedPid), 0) == -1 && errno == ESRCH))
        {
            string processPath = "/proc/" + storedPid + "/cmdline";
            string cmdline;
            ifstream cmd(processPath.c_str());
            // check if process contains name
            if (cmd && cmd >> cmdline && cmdline.find(process) != string::npos)
            {
                LOGM_ERROR(TAG, "Pid associated with active process %s in lockfile: %s", process.c_str(), filename.c_str());

                throw runtime_error{"Device Client is already running."};
            }
        }
        // remove stale pid file
        if (remove(filename.c_str()))
        {
            LOGM_ERROR(TAG, "Unable to remove stale lockfile: %s", filename.c_str());

            throw runtime_error{"File has not been closed."};
        }
    }
    fileIn.close();

    FILE *file = fopen(filename.c_str(), "wx");
    if (!file)
    {
        LOGM_ERROR(TAG, "Unable to open lockfile: %s", filename.c_str());

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