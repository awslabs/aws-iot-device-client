// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_LOGGER_H
#define DEVICE_CLIENT_LOGGER_H

#include "../config/Config.h"
#include "../util/StringUtils.h"
#include "LogLevel.h"
#include "LogQueue.h"
#include <chrono>
#include <cstdarg>
#include <ctime>
#include <memory>
#include <utility>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace LogUtil
            {
                /**
                 * Generates a timestamp to be applied to a log entry
                 * @param t the current time
                 * @param timeBuffer a buffer to store the timestamp in
                 */
                void generateTimestamp(
                    std::chrono::time_point<std::chrono::system_clock> t,
                    size_t bufferSize,
                    char *timeBuffer);
            } // namespace LogUtil

            namespace Logging
            {
                /**
                 * \brief Interface representing essential methods that must provided by any underlying Log generating
                 * implementation
                 *
                 * The Logger class represents an interface by which the Device Client may generate logs and write
                 * them to some desired output. The class itself provides some top level methods that handle logging
                 * levels and formatting log messages with variadic arguments. The underlying logger implementation
                 * handles actual log output generating through use of the queueLog method, which assumes the
                 * implementation will swiftly queue the log message and then handle output accordingly.
                 */
                class Logger
                {
                  protected:
                    const char *LOGGER_TAG = "AWS IoT Device Client Logger";
                    /**
                     * \brief The runtime log level for the IoT Device Client
                     */
                    int logLevel = (int)LogLevel::DEBUG;

                    /**
                     * \brief Implemented by the underlying logger implementation to pass responsibility for managing
                     * the log message from the Logger interface to the logger implementation
                     *
                     * This virtual method should be implemented by the underlying logger implementation to actually
                     * accept and eventually process the incoming log message. To reduce complications induced by
                     * multithreading, the underlying logger implementation should queue the message for processing by
                     * another thread if possible.
                     * @param level the log level
                     * @param tag a tag that indicates where the log message is coming from
                     * @param t a timestamp representing the time the message was created
                     * @param message the message to log
                     */
                    virtual void queueLog(
                        LogLevel level,
                        const char *tag,
                        std::chrono::time_point<std::chrono::system_clock> t,
                        const std::string &message) = 0;

                    /**
                     * \brief Sets the level of the Logger implementation (DEBUG, INFO, WARN, ERROR)
                     *
                     * @param level the level to set the logger to
                     */
                    void setLogLevel(int level) { logLevel = level; }

                  public:
                    // Logger inherited by FileLogger. Make destructor virtual to avoid memory leak.
                    virtual ~Logger() = default;

                    /**
                     * \brief Formats the provided log message against variadic arguments and then
                     * passes the message to the underlying logger implementation for processing
                     * @param level the log level
                     * @param tag a tag that indicates where the log message is coming from
                     * @param t a timestamp representing the time the message was created
                     * @param message the message to log (The message string must be NULL terminated)
                     * @param ... a variadic number of arguments that will be formatted against the log message
                     */
                    virtual void vlog(
                        LogLevel level,
                        const char *tag,
                        std::chrono::time_point<std::chrono::system_clock> t,
                        const char *message,
                        va_list args)
                    {
                        std::string formattedMessage = Util::vFormatMessage(message, args);
                        queueLog(level, tag, t, formattedMessage);
                    }

                    /**
                     * \brief Log the message at the ERROR level. If the current logging level is less than ERROR,
                     * then this is a NOOP.
                     *
                     * @tparam Args variadic number of arguments that may be passed in for formatting against the log
                     * message
                     * @param tag a tag indicating where in the source code the log message is coming from
                     * @param t a timestamp representing the time this message was created
                     * @param message the log message (The message string must be NULL terminated)
                     * @param args variadic number of arguments that may be passed in for formatting against the log
                     * message
                     */
                    void error(
                        const char *tag,
                        std::chrono::time_point<std::chrono::system_clock> t,
                        const char *message,
                        ...)
                    {
                        va_list args;
                        va_start(args, message);
                        if (logLevel >= (int)LogLevel::ERROR)
                        {
                            vlog(LogLevel::ERROR, tag, t, message, args);
                        }
                        va_end(args);
                    }

                    /**
                     * \brief Log the message at the WARN level. If the current logging level is less than WARN,
                     * then this is a NOOP.
                     *
                     * @tparam Args variadic number of arguments that may be passed in for formatting against the log
                     * message
                     * @param tag a tag indicating where in the source code the log message is coming from
                     * @param t a timestamp representing the time this message was created
                     * @param message the log message (The message string must be NULL terminated)
                     * @param args variadic number of arguments that may be passed in for formatting against the log
                     * message
                     */
                    void warn(
                        const char *tag,
                        std::chrono::time_point<std::chrono::system_clock> t,
                        const char *message,
                        ...)
                    {
                        va_list args;
                        va_start(args, message);
                        if (logLevel >= (int)LogLevel::WARN)
                        {
                            vlog(LogLevel::WARN, tag, t, message, args);
                        }
                        va_end(args);
                    }

                    /**
                     * \brief Log the message at the INFO level. If the current logging level is less than INFO,
                     * then this is a NOOP.
                     *
                     * @tparam Args variadic number of arguments that may be passed in for formatting against the log
                     * message
                     * @param tag a tag indicating where in the source code the log message is coming from
                     * @param t a timestamp representing the time this message was created
                     * @param message the log message (The message string must be NULL terminated)
                     * @param args variadic number of arguments that may be passed in for formatting against the log
                     * message
                     */
                    void info(
                        const char *tag,
                        std::chrono::time_point<std::chrono::system_clock> t,
                        const char *message,
                        ...)
                    {
                        va_list args;
                        va_start(args, message);
                        if (logLevel >= (int)LogLevel::INFO)
                        {
                            vlog(LogLevel::INFO, tag, t, message, args);
                        }
                        va_end(args);
                    }

                    /**
                     * \brief Log the message at the DEBUG level. If the current logging level is less than DEBUG,
                     * then this is a NOOP.
                     *
                     * @tparam Args variadic number of arguments that may be passed in for formatting against the log
                     * message
                     * @param tag a tag indicating where in the source code the log message is coming from
                     * @param t a timestamp representing the time this message was created
                     * @param message the log message (The message string must be NULL terminated)
                     * @param args variadic number of arguments that may be passed in for formatting against the log
                     * message
                     */
                    void debug(
                        const char *tag,
                        std::chrono::time_point<std::chrono::system_clock> t,
                        const char *message,
                        ...)
                    {
                        va_list args;
                        va_start(args, message);
                        if (logLevel >= (int)LogLevel::DEBUG)
                        {
                            vlog(LogLevel::DEBUG, tag, t, message, args);
                        }
                        va_end(args);
                    }

                    /**
                     * \brief Starts the underlying logger implementation's logging behavior
                     *
                     * @param config the config data passed in from the CLI and JSON
                     * @return true if it is able to start successfully, false otherwise
                     */
                    virtual bool start(const PlainConfig &config) = 0;

                    /**
                     * \brief Attempts to stop the Logger implementation from writing any additional log messages,
                     * likely to switch to a different logger implementation.
                     */
                    virtual void stop() = 0;
                    /**
                     * \brief Notifies the Logger implementation that any queued logs should be dumped to output and the
                     * logger should shut itself down
                     */
                    virtual void shutdown() = 0;

                    /**
                     * \brief Removes the LogQueue from the logger implementation so it can be passed to another
                     * logger implementation for processing
                     * @return a unique_ptr<LogQueue>
                     */
                    virtual std::unique_ptr<LogQueue> takeLogQueue() = 0;

                    /**
                     * \brief Passes a LogQueue to the logger implementation. Typically used if the logger
                     * implementation is being changed
                     * @param logQueue
                     */
                    virtual void setLogQueue(std::unique_ptr<LogQueue> logQueue) = 0;

                    /**
                     * \brief Flush the log output from the queue synchronously
                     *
                     * This is helpful for scenarios where you need to ensure that the logs are written to disk before
                     * any other activity takes place. Note that this will not block other threads, only the thread
                     * that this is called from
                     */
                    virtual void flush() = 0;
                };
            } // namespace Logging
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_LOGGER_H
