// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "FileUtils.h"
#include "../logging/LoggerFactory.h"

#include <errno.h>
#include <iostream>
#include <limits.h> /* PATH_MAX */
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wordexp.h>

using namespace std;
using namespace Aws::Iot::DeviceClient::Util;
using namespace Aws::Iot::DeviceClient::Logging;

constexpr char FileUtils::TAG[];

// TODO: Sanitize message variables before logging.

int FileUtils::Mkdirs(const char *path)
{
    const size_t len = strlen(path);
    char path_buffer[PATH_MAX];
    char *p;

    errno = 0;

    /* Copy string so its mutable */
    if (len > sizeof(path_buffer) - 1)
    {
        errno = ENAMETOOLONG;
        return -1;
    }
    strcpy(path_buffer, path);

    for (p = path_buffer + 1; *p; p++)
    {
        if (*p == '/')
        {
            /* Temporarily truncate */
            *p = '\0';
            if (mkdir(path_buffer, S_IRWXU) != 0)
            {
                if (errno != EEXIST)
                    return -1;
            }
            *p = '/';
        }
    }
    if (mkdir(path_buffer, S_IRWXU) != 0)
    {
        if (errno != EEXIST)
            return -1;
    }

    return 0;
}

string FileUtils::ExtractParentDirectory(const string &filePath)
{
    const size_t rightMostSlash = filePath.rfind("/");
    if (std::string::npos != rightMostSlash)
    {
        return filePath.substr(0, rightMostSlash + 1);
    }
    else
    {
        return "";
    }
}

string FileUtils::ExtractExpandedPath(const string &filePath)
{
    wordexp_t word;
    wordexp(filePath.c_str(), &word, 0);
    string expandedPath = word.we_wordv[0];
    wordfree(&word);
    return expandedPath;
}

bool FileUtils::StoreValueInFile(string value, string filePath)
{
    ofstream file(filePath);
    if (!file.is_open())
    {
        LOGM_ERROR(FileUtils::TAG, "Unable to open file: '%s'", Sanitize(filePath.c_str()).c_str());
        return false;
    }
    file << value;
    file.close();
    return true;
}

int FileUtils::GetFilePermissions(const std::string &path)
{
    struct stat file_info;
    if (stat(path.c_str(), &file_info) == -1)
    {
        LOGM_ERROR(
            TAG,
            "Failed to stat: %s. Please make sure valid file/dir path is provided.",
            Sanitize(path.c_str()).c_str());
        return false;
    }

    return PermissionsMaskToInt(file_info.st_mode);
}

bool FileUtils::ValidateFileOwnershipPermissions(const std::string &path)
{
    struct stat file_info;
    if (stat(path.c_str(), &file_info) == -1)
    {
        LOGM_ERROR(
            TAG,
            "Failed to stat: %s. Please make sure valid file/dir path is provided.",
            Sanitize(path.c_str()).c_str());
        return false;
    }
    if (getuid() != 0 && getuid() != file_info.st_uid)
    {
        LOGM_ERROR(
            TAG,
            "User does not have the ownership permissions to access the file/dir: %s",
            Sanitize(path.c_str()).c_str());
        return false;
    }
    return true;
}

bool FileUtils::ValidateFilePermissions(const std::string &path, const int filePermissions, bool fatalError)
{
    string expandedPath = ExtractExpandedPath(path.c_str());

    if (fatalError && !ValidateFileOwnershipPermissions(expandedPath))
    {
        return false;
    }
    int actualFilePermissions = FileUtils::GetFilePermissions(expandedPath);
    if (filePermissions != actualFilePermissions)
    {
        if (actualFilePermissions == 0)
        {
            return false;
        }
        if (fatalError)
        {
            LOGM_ERROR(
                TAG,
                "Permissions to given file/dir path '%s' is not set to required value... {Permissions: {desired: %d, "
                "actual: %d}}",
                Sanitize(expandedPath.c_str()).c_str(),
                filePermissions,
                actualFilePermissions);
        }
        else
        {
            LOGM_WARN(
                TAG,
                "Permissions to given file/dir path '%s' is not set to recommended value... {Permissions: {desired: "
                "%d, actual: %d}}",
                Sanitize(expandedPath.c_str()).c_str(),
                filePermissions,
                actualFilePermissions);
        }
        return false;
    }
    return true;
}

int FileUtils::PermissionsMaskToInt(mode_t mask)
{
    int user = 0;
    if (mask & S_IRUSR)
    {
        user += 4;
    }
    if (mask & S_IWUSR)
    {
        user += 2;
    }
    if (mask & S_IXUSR)
    {
        user += 1;
    }

    int group = 0;
    if (mask & S_IRGRP)
    {
        group += 4;
    }
    if (mask & S_IWGRP)
    {
        group += 2;
    }
    if (mask & S_IXGRP)
    {
        group += 1;
    }

    int world = 0;
    if (mask & S_IROTH)
    {
        world += 4;
    }
    if (mask & S_IWOTH)
    {
        world += 2;
    }
    if (mask & S_IXOTH)
    {
        world += 1;
    }

    return (user * 100) + (group * 10) + world;
}

size_t FileUtils::GetFileSize(const std::string &filePath)
{
    string expandedPath = ExtractExpandedPath(filePath.c_str());

    struct stat file_info;
    if (stat(expandedPath.c_str(), &file_info) == 0)
    {
        return file_info.st_size;
    }
    return 0;
}

bool FileUtils::CreateDirectoryWithPermissions(const char *dirPath, mode_t permissions)
{
    const int desiredPermissions = PermissionsMaskToInt(permissions);
    string expandedPath = ExtractExpandedPath(dirPath);

    if (!Mkdirs(expandedPath.c_str()))
    {
        int actualPermissions = GetFilePermissions(expandedPath);
        if (desiredPermissions != actualPermissions)
        {
            chmod(expandedPath.c_str(), permissions);
            actualPermissions = GetFilePermissions(expandedPath);
            if (desiredPermissions != actualPermissions)
            {
                LOGM_ERROR(
                    TAG,
                    "Failed to set appropriate permissions for directory %s, desired %d but found %d",
                    Sanitize(expandedPath.c_str()).c_str(),
                    desiredPermissions,
                    actualPermissions);
                return false;
            }
        }
        else
        {
            LOGM_INFO(
                TAG,
                "Successfully create directory %s with required permissions %d",
                Sanitize(expandedPath.c_str()).c_str(),
                desiredPermissions);
            return true;
        }
    }

    LOGM_ERROR(TAG, "Failed to create directory %s", Sanitize(expandedPath.c_str()).c_str());
    return false;
}