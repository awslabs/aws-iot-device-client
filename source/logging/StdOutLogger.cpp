// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "StdOutLogger.h"

#include <iostream>
#include <sstream>
#include <thread>

constexpr int TIMESTAMP_BUFFER_SIZE = 25;

using namespace std;
using namespace Aws::Iot::DeviceClient::Logging;

constexpr int StdOutLogger::DEFAULT_WAIT_TIME_MILLISECONDS;

void StdOutLogger::writeLogMessage(unique_ptr<LogMessage> message) const
{
    char time_buffer[TIMESTAMP_BUFFER_SIZE];
    LogUtil::generateTimestamp(message->getTime(), TIMESTAMP_BUFFER_SIZE, time_buffer);

    cout << time_buffer << " " << LogLevelMarshaller::ToString(message->getLevel()) << " {" << message->getTag()
         << "}: " << message->getMessage() << endl;
}

void StdOutLogger::run()
{
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

bool StdOutLogger::start(const PlainConfig &config)
{
    setLogLevel(config.logConfig.deviceClientlogLevel);

    thread log_thread(&StdOutLogger::run, this);
    log_thread.detach();

    return true;
}

void StdOutLogger::stop()
{
    needsShutdown = true;
    logQueue->shutdown();
}

unique_ptr<LogQueue> StdOutLogger::takeLogQueue()
{
    unique_ptr<LogQueue> tmp = std::move(logQueue);
    logQueue = unique_ptr<LogQueue>(new LogQueue);
    return tmp;
}

void StdOutLogger::setLogQueue(std::unique_ptr<LogQueue> incomingQueue)
{
    this->logQueue = std::move(incomingQueue);
}

void StdOutLogger::shutdown()
{
    needsShutdown = true;
    logQueue->shutdown();

    // If we've gotten here, we must be shutting down so we should dump the remaining messages and exit
    flush();
}

void StdOutLogger::flush()
{
    while (logQueue->hasNextLog())
    {
        unique_ptr<LogMessage> message = logQueue->getNextLog();
        if (nullptr != message)
        {
            writeLogMessage(std::move(message));
        }
    }
}

void StdOutLogger::queueLog(
    LogLevel level,
    const char *tag,
    std::chrono::time_point<std::chrono::system_clock> t,
    const std::string &message)
{
    logQueue.get()->addLog(unique_ptr<LogMessage>(new LogMessage(level, tag, t, message)));
}
