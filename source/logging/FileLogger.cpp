// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "FileLogger.h"

#include <errno.h>
#include <iostream>
#include <limits.h> /* PATH_MAX */
#include <string.h>
#include <sys/stat.h> /* mkdir(2) */
#include <thread>

#define TIMESTAMP_BUFFER_SIZE 25

using namespace std;
using namespace Aws::Iot::DeviceClient::Logging;

bool FileLogger::start()
{
    struct stat info;
    bool initialized = false;

    if (stat(DEFAULT_LOG_DIR.c_str(), &info) != 0)
    {
        cout << LOGGER_TAG << ": Cannot access " << DEFAULT_LOG_DIR
             << "to write logs, attempting to create log directory" << endl;
        mkdirs(DEFAULT_LOG_DIR.c_str());
        if (stat(DEFAULT_LOG_DIR.c_str(), &info) != 0)
        {
            cout << LOGGER_TAG << ": Failed to create log directories necessary for file-based logging" << endl;
        }
        else
        {
            cout << LOGGER_TAG << ": Successfully created log directory! Now logging to " << DEFAULT_LOG_FILE << endl;
            initialized = true;
        }
    }
    else if (info.st_mode & S_IFDIR)
    {
        // Log directory already exists, nothing to do here
        initialized = true;
    }
    else
    {
        cout << LOGGER_TAG << ": Unknown condition encountered while trying to create log directory" << endl;
    }

    if (initialized)
    {
        outputStream = unique_ptr<ofstream>(new ofstream(DEFAULT_LOG_FILE, std::fstream::app));

        thread log_thread(&FileLogger::run, this);
        log_thread.detach();
        return true;
    }

    return false;
}

void FileLogger::writeLogMessage(unique_ptr<LogMessage> message)
{
    char time_buffer[TIMESTAMP_BUFFER_SIZE];
    LogUtil::generateTimestamp(message->getTime(), TIMESTAMP_BUFFER_SIZE, time_buffer);

    *outputStream << time_buffer << " " << LogLevelMarshaller::ToString(message->getLevel()) << " {"
                  << message->getTag() << "}: " << message->getMessage() << endl;
    outputStream->flush();
}

void FileLogger::run()
{
    while (!needsShutdown)
    {
        unique_ptr<LogMessage> message = logQueue->getNextLog();

        if (NULL != message)
        {
            writeLogMessage(std::move(message));
        }
    }
}

void FileLogger::queueLog(
    LogLevel level,
    const char *tag,
    std::chrono::time_point<std::chrono::system_clock> t,
    string message)
{
    logQueue.get()->addLog(unique_ptr<LogMessage>(new LogMessage(level, tag, t, message)));
}

void FileLogger::shutdown()
{
    needsShutdown = true;
    logQueue->shutdown();

    // If we've gotten here, we must be shutting down so we should dump the remaining messages and exit
    flush();
}

void FileLogger::flush()
{
    while (logQueue->hasNextLog())
    {
        unique_ptr<LogMessage> message = logQueue->getNextLog();
        writeLogMessage(std::move(message));
    }
}

int FileLogger::mkdirs(const char *path)
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
