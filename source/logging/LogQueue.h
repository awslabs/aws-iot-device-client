// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_LOGQUEUE_H
#define DEVICE_CLIENT_LOGQUEUE_H

#include "LogMessage.h"
#include <queue>
#include <condition_variable>
#include <mutex>

namespace Aws {
    namespace Iot {
        namespace DeviceClient {
            class LogQueue {
            private:
                bool isShutdown = false;
                std::mutex queueLock;
                std::condition_variable newLogNotifier;
                std::queue<std::unique_ptr<LogMessage>> logQueue;
            public:
                /**
                  * Adds a single log to the LogQueue.
                  * @param log the log to add to the LogQueue
                  */
                void addLog(std::unique_ptr<LogMessage> log);
                /**
                 * Gets the next log message.
                 * @return the next log message in the LogQueue
                 */
                std::unique_ptr<LogMessage> getNextLog();

                /**
                 * Determine whether the LogQueue has a message available
                 * @return true if there is a message present, false otherwise
                 */
                bool hasNextLog();

                /**
                 * This function essentially shuts off any of the 'waiting' behavior when it comes to
                 * getting the next event in the LogQueue. It will force the getNextEvent() to return
                 * whether there is a log message or not
                 */
                void shutdown();
            };
        }
    }
}


#endif //DEVICE_CLIENT_LOGQUEUE_H
