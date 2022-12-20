// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_STDOUTLOGGER_H
#define DEVICE_CLIENT_STDOUTLOGGER_H

#include "LogLevel.h"
#include "LogQueue.h"
#include "Logger.h"

#include <memory>
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
                 * \brief Logging implementation that writes log messages directly to STDOUT
                 */
                class StdOutLogger final : public Logger
                {
                    /**
                     * \brief Flag used to notify underlying threads that they should discontinue any processing
                     * so that the application can safely shutdown
                     */
                    bool needsShutdown = false;
                    /**
                     * \brief The default value in milliseconds for which Device client will wait after getting a
                     * log message from logQueue
                     */
                    static constexpr int DEFAULT_WAIT_TIME_MILLISECONDS = 1;
                    /**
                     * \brief a LogQueue instance used to queue incoming log messages for processing
                     */
                    std::unique_ptr<LogQueue> logQueue = std::unique_ptr<LogQueue>(new LogQueue);
                    /**
                     * \brief Begins processing of log messages in the LogQueue
                     *
                     * This method will begin processing of log messages in the LogQueue. The thread will process until
                     * all of the messages are removed from the queue, and then will wait until new messages arrive in
                     * the queue. This method will check to make sure the shutdown() method has not been called before
                     * processing any additional messages in the queue.
                     */
                    void run();
                    /**
                     * \brief Write the log message to standard output
                     *
                     * This method will write the log message to the file specified for logging
                     * @param message the message to log
                     */
                    void writeLogMessage(std::unique_ptr<LogMessage> message) const;

                  protected:
                    virtual void queueLog(
                        LogLevel level,
                        const char *tag,
                        std::chrono::time_point<std::chrono::system_clock> t,
                        const std::string &message) override;

                  public:
                    virtual bool start(const PlainConfig &config) override;

                    virtual void stop() override;

                    virtual void shutdown() override;

                    virtual std::unique_ptr<LogQueue> takeLogQueue() override;

                    virtual void setLogQueue(std::unique_ptr<LogQueue> logQueue) override;

                    virtual void flush() override;
                };
            } // namespace Logging
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_STDOUTLOGGER_H
