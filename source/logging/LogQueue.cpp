// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "LogQueue.h"

using namespace std;
using namespace Aws::Iot::DeviceClient::Logging;

void LogQueue::addLog(unique_ptr<LogMessage> log)
{
    unique_lock<mutex> addLock(queueLock);
    logQueue.push_back(std::move(log));
    addLock.unlock();
    newLogNotifier.notify_one();
}

bool LogQueue::hasNextLog()
{
    unique_lock<mutex> readLock(queueLock);
    return !logQueue.empty();
}

std::unique_ptr<LogMessage> LogQueue::getNextLog()
{
    unique_lock<mutex> readLock(queueLock);
    while (logQueue.empty() && !isShutdown)
    {
        newLogNotifier.wait(readLock);
    }
    if (logQueue.empty())
    {
        return NULL;
    }
    else
    {
        unique_ptr<LogMessage> message = std::move(logQueue.front());
        logQueue.pop_front();
        return message;
    }
}

void LogQueue::shutdown()
{
    // Grab the lock in case there's active logging while we attempt to shutdown
    unique_lock<mutex> shutdownLock(queueLock);
    // We need to prepend the queue with a null message so that any waiting threads are interrupted and do not process
    // any of the log messages.
    logQueue.push_front(NULL);

    isShutdown = true;

    // Force getNextEvent() to stop blocking regardless of whether there's actually a new event
    // so that we can safely shutdown
    newLogNotifier.notify_one();
}
