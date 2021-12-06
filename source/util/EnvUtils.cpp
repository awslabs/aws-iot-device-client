// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "EnvUtils.h"
#include "../logging/LoggerFactory.h"

#include <cerrno>
#include <cstdlib>
#include <sstream>
#include <vector>

#include <linux/limits.h>
#include <string.h>
#include <unistd.h>

using namespace Aws::Iot::DeviceClient::Util;
using namespace Aws::Iot::DeviceClient::Logging;

char *OSInterfacePosix::getenv(const char *name)
{
    return ::getenv(name);
}

int OSInterfacePosix::setenv(const char *name, const char *value, int overwrite)
{
    return ::setenv(name, value, overwrite);
}

char *OSInterfacePosix::getcwd(char *buf, size_t size)
{
    return ::getcwd(buf, size);
}

// Sequences of path prefixes used to search for executable filenames.
constexpr char PATH_ENVIRONMENT[] = "PATH";

// Separator between path prefixes in environment variable.
constexpr char PATH_ENVIRONMENT_SEPARATOR = ':'; // Unix-only.

// Separator between directories in path.
constexpr char PATH_DIRECTORY_SEPARATOR = '/'; // Unix-only.

// Jobs directory name.
constexpr char JOBS_DIRECTORY_NAME[] = "jobs";

constexpr char TAG[] = __FILE__;

int EnvUtils::AppendCwdToPath()
{
    // Use a vector of char as buffer for path to current working directory.
    //
    // Maximum number of characters in absolute path is filesystem dependent.
    //
    // The getcwd function takes a buffer and length and returns the current
    // working directory or NULL if the length of the buffer is insufficient
    // to store the current working directory.
    //
    // As a result, we initialize the buffer with the maximum length of a
    // relative pathname (_PC_PATH_MAX) and then repeatedly resize the buffer
    // in the event that getcwd is unable to fit the current working directory
    // in the buffer passed as input.
    std::vector<char> cwd;
    long path_max = pathconf(".", _PC_PATH_MAX);
    if (path_max < 1)
    {
        cwd.resize(PATH_MAX + 1, '\0'); // Fallback to hardcoded PATH_MAX.
    }
    else
    {
        cwd.resize(path_max + 1, '\0');
    }

    char *pcwd = nullptr;
    while (!pcwd)
    {
        pcwd = os->getcwd(cwd.data(), cwd.size() - 1); // Leave room for terminating null.
        if (!pcwd)
        {
            if (errno == ERANGE)
            {
                cwd.resize(cwd.size() * 2, '\0');
            }
            else
            {
                int errnum = errno != 0 ? errno : 1;
                LOGM_ERROR(TAG, "Unable to get current working directory errno: %d msg: %s", errnum, strerror(errnum));
                return errnum;
            }
        }
    }

    // Append standard paths used by device client based on current working directory.
    std::ostringstream oss;
    if (const char *path = os->getenv(PATH_ENVIRONMENT))
    {
        oss << path;
    }
    if (oss.tellp())
    {
        oss << PATH_ENVIRONMENT_SEPARATOR;
    }
    oss << cwd.data() // Current working directory.
        << PATH_ENVIRONMENT_SEPARATOR << cwd.data() << PATH_DIRECTORY_SEPARATOR
        << JOBS_DIRECTORY_NAME; // Jobs subdirectory.

    // Overwrite path environment variable.
    const std::string &newpath = oss.str();
    if (os->setenv(PATH_ENVIRONMENT, newpath.c_str(), 1) != 0)
    {
        int errnum = errno != 0 ? errno : 1;
        LOGM_ERROR(
            TAG,
            "Unable to overwrite %s environment variable errno: %d msg: %s",
            PATH_ENVIRONMENT,
            errnum,
            strerror(errnum));
        return errnum;
    }
    LOGM_DEBUG(TAG, "Updated %s environment variable to: %s", PATH_ENVIRONMENT, newpath.c_str());

    return 0;
}
