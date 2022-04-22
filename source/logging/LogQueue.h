// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_LOGQUEUE_H
#define DEVICE_CLIENT_LOGQUEUE_H

#include "LogMessage.h"
#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Logging
            {
                /**
                 * \brief A thread-safe queue used by our Logger implementations to queue incoming messages
                 * from multiple threads and process them in order
                 */
                class LogQueue
                {
                  private:
                    /**
                     * \brief Whether the LogQueue has been shutdown or not.
                     */
                    std::atomic<bool> isShutdown{false};
                    /**
                     * \brief The default value in milliseconds for which Device client will wait after blocking when
                     * the queue is empty.
                     */
                    static constexpr int EMPTY_WAIT_TIME_MILLISECONDS = 200;
                    /**
                     * \brief a Mutex used to control multi-threaded access to the LogQueue
                     */
                    std::mutex queueLock;
                    /**
                     * \brief Used to wake up waiting threads when new data arrives, or when
                     * the LogQueue has shut down
                     */
                    std::condition_variable newLogNotifier;
                    /**
                     * \brief Responsible for queuing the LogMessages upon arrival for processing
                     */
                    std::deque<std::unique_ptr<LogMessage>> logQueue;

                  public:
                    /**
                     * \brief Adds a single log to the LogQueue.
                     *
                     * @param log the log to add to the LogQueue
                     */
                    void addLog(std::unique_ptr<LogMessage> log);
                    /**
                     * \brief Gets the next log message.
                     *
                     * @return the next log message in the LogQueue
                     */
                    std::unique_ptr<LogMessage> getNextLog();

                    /**
                     * \brief Determine whether the LogQueue has a message available
                     *
                     * @return true if there is a message present, false otherwise
                     */
                    bool hasNextLog();

                    /**
                     * \brief Force all consumers to stop waiting so that they can flush the queue
                     * and end any waiting behavior that might prevent the thread from shutting down.
                     *
                     * This function essentially shuts off any of the 'waiting' behavior when it comes to
                     * getting the next message in the LogQueue. It will force the getNextLog() method to return
                     * whether there is a log message or not
                     */
                    void shutdown();
                };
            } // namespace Logging
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_LOGQUEUE_H
