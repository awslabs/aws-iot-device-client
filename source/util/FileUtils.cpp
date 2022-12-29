// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "FileUtils.h"
#include "../logging/LoggerFactory.h"

#include <errno.h>
#include <iostream>

#include <fcntl.h>
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

int FileUtils::Mkdirs(const std::string &path)
{
    if (path.length() < 1)
    {
        return -1;
    }
    for (size_t i = 1; i < path.length(); i++)
    {
        if (path[i] == '/' && mkdir(path.substr(0, i).c_str(), S_IRWXU) != 0 && errno != EEXIST)
        {
            return -1;
        }
    }
    if (mkdir(path.c_str(), S_IRWXU) != 0 && errno != EEXIST)
    {
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
        return "./";
    }
}

string FileUtils::ExtractExpandedPath(const string &filePath)
{
    if (filePath.empty())
    {
        return "";
    }
    wordexp_t word;
    int status = wordexp(filePath.c_str(), &word, 0);
    if (status)
    {
        throw wordexp_fail_error(
            "Received status from wordexp: " + std::to_string(status) +
            " Pertaining to following filepath: " + filePath);
    }
    string expandedPath = word.we_wordv[0];
    wordfree(&word);
    return expandedPath;
}

bool FileUtils::StoreValueInFile(const string &value, const string &filePath)
{
    ofstream file(filePath);
    if (!file.is_open())
    {
        LOGM_ERROR(FileUtils::TAG, "Unable to open file: '%s'", Sanitize(filePath).c_str());
        return false;
    }
    file << value;
    file.close();
    return true;
}

int FileUtils::ReadFromFile(const std::string &pathToFile, aws_byte_buf *data, size_t size)
{
    ifstream file(pathToFile);
    if (data->capacity < size)
    {
        LOGM_ERROR(FileUtils::TAG, "Allocated read buffer smaller than size: %zu < %zu", data->capacity, size);
        return AWS_OP_ERR;
    }
    if (!file.is_open())
    {
        LOGM_ERROR(FileUtils::TAG, "Unable to open file: '%s'", Sanitize(pathToFile).c_str());
        return AWS_OP_ERR;
    }
    file.read(reinterpret_cast<char *>(data->buffer), size);
    data->len = size;

    if (!file)
    {
        file.close();
        return AWS_OP_ERR;
    }
    else
    {
        file.close();
        return AWS_OP_SUCCESS;
    }
}

int FileUtils::WriteToFile(const std::string &pathToFile, const aws_byte_buf *data)
{
    ofstream file(pathToFile, std::ios::app);
    if (!file.is_open())
    {
        LOGM_ERROR(FileUtils::TAG, "Unable to open file: '%s'", Sanitize(pathToFile).c_str());
        return AWS_OP_ERR;
    }
    file.write(reinterpret_cast<char *>(data->buffer), data->len);
    if (!file)
    {
        file.close();
        return AWS_OP_ERR;
    }
    else
    {
        file.close();
        return AWS_OP_SUCCESS;
    }
}

int FileUtils::GetFilePermissions(const std::string &path)
{
    struct stat file_info;
    if (stat(path.c_str(), &file_info) == -1)
    {
        LOGM_ERROR(
            TAG, "Failed to stat: %s. Please make sure valid file/dir path is provided.", Sanitize(path).c_str());
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
            TAG, "Failed to stat: %s. Please make sure valid file/dir path is provided.", Sanitize(path).c_str());
        return false;
    }
    if (getuid() != 0 && getuid() != file_info.st_uid)
    {
        LOGM_ERROR(
            TAG, "User does not have the ownership permissions to access the file/dir: %s", Sanitize(path).c_str());
        return false;
    }
    return true;
}

bool FileUtils::ValidateFilePermissions(const std::string &path, const int filePermissions, bool fatalError)
{
    string expandedPath = ExtractExpandedPath(path);

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
                Sanitize(expandedPath).c_str(),
                filePermissions,
                actualFilePermissions);
        }
        else
        {
            LOGM_WARN(
                TAG,
                "Permissions to given file/dir path '%s' is not set to recommended value... {Permissions: {desired: "
                "%d, actual: %d}}",
                Sanitize(expandedPath).c_str(),
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
    string expandedPath = ExtractExpandedPath(filePath);

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

    if (!Mkdirs(expandedPath))
    {
        int actualPermissions = GetFilePermissions(expandedPath);
        if (desiredPermissions != actualPermissions)
        {
            chmod(expandedPath.c_str(), permissions);
            // Repeat permission check for verification.
            actualPermissions = GetFilePermissions(expandedPath);
            // cppcheck-suppress knownConditionTrueFalse
            if (desiredPermissions != actualPermissions)
            {
                LOGM_ERROR(
                    TAG,
                    "Failed to set appropriate permissions for directory %s, desired %d but found %d",
                    Sanitize(expandedPath).c_str(),
                    desiredPermissions,
                    actualPermissions);
                return false;
            }
            else
            {
                // Desired and actual permissions match.
                LOGM_INFO(
                    TAG,
                    "Successfully create directory %s with required permissions %d",
                    Sanitize(expandedPath).c_str(),
                    desiredPermissions);
                return true;
            }
        }
        else
        {
            LOGM_INFO(
                TAG,
                "Successfully create directory %s with required permissions %d",
                Sanitize(expandedPath).c_str(),
                desiredPermissions);
            return true;
        }
    }

    LOGM_ERROR(TAG, "Failed to create directory %s", Sanitize(expandedPath).c_str());
    return false;
}

bool FileUtils::DirectoryExists(const std::string &dirPath)
{
    auto expandedDirPath = ExtractExpandedPath(dirPath);
    struct stat dirInfo;
    if (stat(expandedDirPath.c_str(), &dirInfo) != 0)
    {
        // Ignore permission errors.
        return false;
    }
    return S_ISDIR(dirInfo.st_mode);
}

bool FileUtils::CreateEmptyFileWithPermissions(const string &filename, mode_t permissions)
{
    auto expandedFilename = ExtractExpandedPath(filename);
    auto fd = open(expandedFilename.c_str(), O_CREAT | O_EXCL, permissions);
    if (-1 == fd)
    {
        auto errnum = errno;
        LOGM_ERROR(
            TAG,
            "Failed to create empty file: %s errno: %d msg: %s",
            Sanitize(expandedFilename).c_str(),
            errnum,
            strerror(errnum));
        return false;
    }
    close(fd);
    return true;
}

bool FileUtils::FileExists(const string &filename)
{
    string expandedPath = FileUtils::ExtractExpandedPath(filename);
    ifstream f(expandedPath);
    return f.good();
}

bool FileUtils::IsValidFilePath(const string &filePath)
{
    wordexp_t word;
    switch (wordexp(filePath.c_str(), &word, 0))
    {
        case 0:
            break;
        case WRDE_NOSPACE:
            wordfree(&word);
            break;
        default:
            return false;
    }

    string expandedPath = word.we_wordv[0];

    if (!FileUtils::FileExists(expandedPath))
    {
        wordfree(&word);
        return false;
    }
    wordfree(&word);
    return true;
}
