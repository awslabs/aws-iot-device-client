// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_FILELOGGER_H
#define DEVICE_CLIENT_FILELOGGER_H

#include <fstream>
#include <memory>
#include <mutex>
#include <stdio.h>

#include "LogLevel.h"
#include "LogQueue.h"
#include "Logger.h"

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Logging
            {
                /**
                 * \brief File-based logging implementation for writing log messages to a file on the device
                 */
                class FileLogger final : public Logger
                {
                  private:
                    /**
                     * \brief Runtime configuration for which log file to log to.
                     */
                    std::string logFile = DEFAULT_LOG_FILE;

                    /**
                     * \brief The default value in milliseconds for which Device client will wait after getting a
                     * log message from logQueue
                     */
                    static constexpr int DEFAULT_WAIT_TIME_MILLISECONDS = 1;

                    /**
                     * \brief Flag used to notify underlying threads that they should discontinue any processing
                     * so that the application can safely shutdown
                     */
                    bool needsShutdown = false;

                    std::mutex isRunningLock;
                    bool isRunning = false;
                    /**
                     * \brief a LogQueue instance used to queue incoming log messages for processing
                     */
                    std::unique_ptr<LogQueue> logQueue = std::unique_ptr<LogQueue>(new LogQueue);

                    /**
                     * \brief an std::ofstream representing an underlying file that is used to write log output to disk
                     */
                    std::unique_ptr<std::ofstream> outputStream;

                    /**
                     * \brief Write the log message to the log file
                     *
                     * This method will write the log message to the file specified for logging
                     * @param message the message to log
                     */
                    void writeLogMessage(std::unique_ptr<LogMessage> message) const;

                    /**
                     * \brief Creates the directories required as part of the full path to the desired log file
                     *
                     * This method is run as part of the initialization process of the FileLogger implementation
                     * to create any directories required as part of the full path to the desired log file
                     */
                    void createLogDirectories();

                    /**
                     * \brief Begins processing of log messages in the LogQueue
                     *
                     * This method will begin processing of log messages in the LogQueue. The thread will process until
                     * all of the messages are removed from the queue, and then will wait until new messages arrive in
                     * the queue. This method will check to make sure the shutdown() method has not been called before
                     * processing any additional messages in the queue.
                     */
                    void run();

                    virtual void queueLog(
                        LogLevel level,
                        const char *tag,
                        std::chrono::time_point<std::chrono::system_clock> t,
                        const std::string &message) override;

                  public:
                    /**
                     * \brief The full path to the default log file for the Device Client
                     *
                     * If the user does not specify a desired log location in either the command line arguments
                     * or the Json configuration file, this is the default log that will be used
                     */
                    static constexpr char DEFAULT_LOG_FILE[] =
                        "/var/log/aws-iot-device-client/aws-iot-device-client.log";

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

#endif // DEVICE_CLIENT_FILELOGGER_H
