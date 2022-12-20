// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "FileLogger.h"
#include "../util/FileUtils.h"

#include <iostream>
#include <sys/stat.h> /* mkdir(2) */
#include <thread>

constexpr int TIMESTAMP_BUFFER_SIZE = 25;

using namespace std;
using namespace Aws::Iot::DeviceClient::Logging;
using namespace Aws::Iot::DeviceClient::Util;

constexpr int FileLogger::DEFAULT_WAIT_TIME_MILLISECONDS;
constexpr char FileLogger::DEFAULT_LOG_FILE[];

bool FileLogger::start(const PlainConfig &config)
{
    setLogLevel(config.logConfig.deviceClientlogLevel);
    if (!config.logConfig.deviceClientLogFile.empty())
    {
        logFile = config.logConfig.deviceClientLogFile;
    }

    struct stat info;
    string logFileDir = FileUtils::ExtractParentDirectory(logFile);
    if (stat(logFileDir.c_str(), &info) != 0)
    {
        cout << LOGGER_TAG << ": Cannot access " << logFileDir << "to write logs, attempting to create log directory"
             << endl;
        FileUtils::Mkdirs(logFileDir);
        if (stat(logFileDir.c_str(), &info) != 0)
        {
            cout << LOGGER_TAG << ": Failed to create log directories necessary for file-based logging" << endl;
            return false;
        }
        else
        {
            cout << LOGGER_TAG << ": Successfully created log directory! Now logging to " << logFile << endl;
        }
    }
    else if (info.st_mode & S_IFDIR)
    {
        // Log directory already exists, nothing to do here
    }
    else
    {
        cout << LOGGER_TAG << ": Unknown condition encountered while trying to create log directory" << endl;
        return false;
    }

    // Now we need to establish/verify permissions for the log directory and file
    if (Permissions::LOG_DIR != FileUtils::GetFilePermissions(logFileDir))
    {
        chmod(logFileDir.c_str(), S_IRWXU | S_IRGRP | S_IROTH | S_IXOTH);
        if (Permissions::LOG_DIR != FileUtils::GetFilePermissions(logFileDir))
        {
            cout << LOGGER_TAG
                 << FormatMessage(
                        "Failed to set appropriate permissions for log file directory %s, permissions should be set to "
                        "%d",
                        logFileDir.c_str(),
                        Permissions::LOG_DIR);
        }
    }

    outputStream = unique_ptr<ofstream>(new ofstream(logFile, std::fstream::app));
    if (!outputStream->fail())
    {
        if (Permissions::LOG_FILE != FileUtils::GetFilePermissions(logFile))
        {
            chmod(logFile.c_str(), S_IRUSR | S_IWUSR);
            if (Permissions::LOG_FILE != FileUtils::GetFilePermissions(logFile))
            {
                cout << LOGGER_TAG
                     << FormatMessage(
                            "Failed to set appropriate permissions for log file %s, permissions should be set to %d",
                            logFile.c_str(),
                            Permissions::LOG_FILE);
            }
        }

        thread log_thread(&FileLogger::run, this);
        log_thread.detach();
        return true;
    }

    cout << LOGGER_TAG << FormatMessage(": Failed to open %s for logging", logFile.c_str()) << endl;
    return false;
}

void FileLogger::writeLogMessage(unique_ptr<LogMessage> message) const
{
    char time_buffer[TIMESTAMP_BUFFER_SIZE];
    LogUtil::generateTimestamp(message->getTime(), TIMESTAMP_BUFFER_SIZE, time_buffer);

    *outputStream << time_buffer << " " << LogLevelMarshaller::ToString(message->getLevel()) << " {"
                  << message->getTag() << "}: " << message->getMessage() << endl;
    outputStream->flush();
}

void FileLogger::run()
{
    unique_lock<mutex> runLock(isRunningLock);
    isRunning = true;
    runLock.unlock();

    while (!needsShutdown)
    {
        unique_ptr<LogMessage> message = logQueue->getNextLog();

        if (nullptr != message)
        {
            writeLogMessage(std::move(message));
        }
        this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_WAIT_TIME_MILLISECONDS));
    }
}

void FileLogger::queueLog(
    LogLevel level,
    const char *tag,
    std::chrono::time_point<std::chrono::system_clock> t,
    const string &message)
{
    logQueue.get()->addLog(unique_ptr<LogMessage>(new LogMessage(level, tag, t, message)));
}

void FileLogger::stop()
{
    needsShutdown = true;
    logQueue->shutdown();

    unique_lock<mutex> runLock(isRunningLock);
    isRunning = false;
}

unique_ptr<LogQueue> FileLogger::takeLogQueue()
{
    unique_ptr<LogQueue> tmp = std::move(logQueue);
    logQueue = unique_ptr<LogQueue>(new LogQueue);
    return tmp;
}

void FileLogger::setLogQueue(std::unique_ptr<LogQueue> incomingQueue)
{
    this->logQueue = std::move(incomingQueue);
}

void FileLogger::shutdown()
{
    needsShutdown = true;
    logQueue->shutdown();

    // If we've gotten here, we must be shutting down so we should dump the remaining messages and exit
    flush();

    unique_lock<mutex> runLock(isRunningLock);
    isRunning = false;
}

void FileLogger::flush()
{
    unique_lock<mutex> runLock(isRunningLock);
    if (!isRunning)
    {
        return;
    }
    runLock.unlock();

    while (logQueue->hasNextLog())
    {
        unique_ptr<LogMessage> message = logQueue->getNextLog();
        if (nullptr != message)
        {
            writeLogMessage(std::move(message));
        }
    }
}