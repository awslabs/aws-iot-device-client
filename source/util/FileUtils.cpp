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
    wordexp_t expandedPath;
    wordexp(filePath.c_str(), &expandedPath, 0);

    struct stat file_info;
    if (stat(expandedPath.we_wordv[0], &file_info) == -1)
    {
        LOGM_ERROR(TAG, "Failed to stat %s", expandedPath.we_wordv[0]);
        return false;
    }

    int user = 0;
    if (file_info.st_mode & S_IRUSR)
    {
        user += 4;
    }
    if (file_info.st_mode & S_IWUSR)
    {
        user += 2;
    }
    if (file_info.st_mode & S_IXUSR)
    {
        user += 1;
    }

    int group = 0;
    if (file_info.st_mode & S_IRGRP)
    {
        group += 4;
    }
    if (file_info.st_mode & S_IWGRP)
    {
        group += 2;
    }
    if (file_info.st_mode & S_IXGRP)
    {
        group += 1;
    }

    int world = 0;
    if (file_info.st_mode & S_IROTH)
    {
        world += 4;
    }
    if (file_info.st_mode & S_IWOTH)
    {
        world += 2;
    }
    if (file_info.st_mode & S_IXOTH)
    {
        world += 1;
    }

    return (user * 100) + (group * 10) + world;
}

size_t FileUtils::getFileSize(const std::string &filePath)
{
    wordexp_t expandedPath;
    wordexp(filePath.c_str(), &expandedPath, 0);

    struct stat file_info;
    if (stat(expandedPath.we_wordv[0], &file_info) == 0)
    {
        return file_info.st_size;
    }

    return 0;
}