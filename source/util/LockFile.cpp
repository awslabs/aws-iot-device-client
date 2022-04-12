// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "LockFile.h"
#include <stdexcept>
#include <csignal>
#include <unistd.h>
#include <fstream>
#include <sstream>

using namespace std;
using namespace Aws::Iot::DeviceClient::Util;

constexpr char LockFile::TAG[];

LockFile::LockFile(const std::string &filename) : filename(filename), file(fopen(filename.c_str(), "r"))
{
    if (file)
    {
        bool running = false;

        ifstream fileIn(filename);
        string storedPid;
        if (fileIn && fileIn >> storedPid)
        {
            // sets flag if process exists
            if (!(kill(stoi(storedPid), 0) == -1 && errno == ESRCH))
            {
                string path = "/proc/" + storedPid + "/cmdline";
                string cmdline;
                ifstream cmd(path.c_str());
                // check if process contains name
                if (cmd >> cmdline && cmdline.find("aws-iot-device-client") != string::npos)
                {
                    running = true;
                }
                cmd.close();
            }
        }
        if (running)
        {
            std::ostringstream oss;
            oss << "Unable to open lockfile "
                << filename
                << "Device client is already running.";
            throw std::runtime_error{oss.str()};
        }
        else
        {
            // remove stale pid file
            remove(filename.c_str());
        }
        fileIn.close();
        fclose(file);
    }

    file = fopen(filename.c_str(), "wx");

    if (!file)
    {
        std::ostringstream oss;
        oss << "Unable to open lockfile "
            << filename
            << "Can not write to file.";
        throw std::runtime_error{oss.str()};
    }
    lockf(fileno(file), F_LOCK, 0);
    string pid = to_string(getpid());
    fputs(pid.c_str(), file);
    fclose(file);
}

LockFile::~LockFile()
{
    remove(filename.c_str());
}