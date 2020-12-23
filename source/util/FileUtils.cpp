#include "FileUtils.h"
#include "../logging/LoggerFactory.h"

#include <errno.h>
#include <fstream>
#include <limits.h> /* PATH_MAX */
#include <string.h>
#include <sys/stat.h>
#include <wordexp.h>

using namespace std;
using namespace Aws::Iot::DeviceClient::Util;
using namespace Aws::Iot::DeviceClient::Logging;

constexpr char FileUtils::TAG[];

int FileUtils::mkdirs(const char *path)
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

string FileUtils::extractParentDirectory(const string &filePath)
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

bool FileUtils::StoreValueInFile(string value, string filePath)
{
    ofstream file(filePath);
    if (!file.is_open())
    {
        LOGM_ERROR(FileUtils::TAG, "Unable to open file: '%s'", filePath.c_str());
        return false;
    }
    file << value;
    file.close();
    return true;
}

int FileUtils::getFilePermissions(const std::string &filePath)
{
    wordexp_t word;
    wordexp(filePath.c_str(), &word, 0);
    string expandedPath = word.we_wordv[0];
    wordfree(&word);

    struct stat file_info;
    if (stat(expandedPath.c_str(), &file_info) == -1)
    {
        LOGM_ERROR(TAG, "Failed to stat %s", expandedPath.c_str());
        return false;
    }

    return permissionsMaskToInt(file_info.st_mode);
}

int FileUtils::permissionsMaskToInt(mode_t mask)
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

size_t FileUtils::getFileSize(const std::string &filePath)
{
    wordexp_t word;
    wordexp(filePath.c_str(), &word, 0);
    string expandedPath = word.we_wordv[0];
    wordfree(&word);

    struct stat file_info;
    if (stat(expandedPath.c_str(), &file_info) == 0)
    {
        return file_info.st_size;
    }
    return 0;
}

bool FileUtils::createDirectoryWithPermissions(const char *dirPath, mode_t permissions)
{
    const int desiredPermissions = permissionsMaskToInt(permissions);
    wordexp_t word;
    wordexp(dirPath, &word, 0);
    string expandedPath = word.we_wordv[0];
    wordfree(&word);

    if (!mkdirs(expandedPath.c_str()))
    {
        int actualPermissions = getFilePermissions(expandedPath);
        if (desiredPermissions != actualPermissions)
        {
            chmod(expandedPath.c_str(), permissions);
            actualPermissions = getFilePermissions(expandedPath);
            if (desiredPermissions != actualPermissions)
            {
                LOGM_ERROR(
                    TAG,
                    "Failed to set appropriate permissions for directory %s, desired %d but found %d",
                    expandedPath.c_str(),
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
                expandedPath.c_str(),
                desiredPermissions);
            return true;
        }
    }

    LOGM_ERROR(TAG, "Failed to create directory %s", expandedPath.c_str());
    return false;
}