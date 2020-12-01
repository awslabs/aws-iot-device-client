// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_LOGGER_H
#define DEVICE_CLIENT_LOGGER_H

#include "../util/StringUtils.h"
#include "LogLevel.h"
#include <chrono>
#include <cstdarg>
#include <ctime>
#include <memory>
#include <utility>

namespace Aws {
    namespace Iot {
        namespace DeviceClient {
            namespace LogUtil {
                /**
                 * Generates a timestamp to be applied to a log entry
                 * @param t the current time
                 * @param timeBuffer a buffer to store the timestamp in
                 */
                void generateTimestamp(std::chrono::time_point<std::chrono::system_clock> t, size_t bufferSize, char * timeBuffer);
            }

            class Logger {
            protected:
                // This value is currently set for testing, but we'll want this to be configurable
                const int DC_LOG_LEVEL = 3;
                const char * LOGGER_TAG = "AWS IoT Device Client Logger";
                virtual void queueLog(LogLevel level, const char * tag, std::chrono::time_point<std::chrono::system_clock> t, std::string message) = 0;
            public:
                virtual void vlog(
                    LogLevel level,
                    const char *tag,
                    std::chrono::time_point<std::chrono::system_clock> t,
                    const char *message,
                    va_list args)
                {
                    std::string formattedMessage = vFormatMessage(message, args);
                    queueLog(level, tag, t, formattedMessage);
                }

                void error(
                    const char *tag,
                    std::chrono::time_point<std::chrono::system_clock> t,
                    const char *message,
                    ...)
                {
                    va_list args;
                    va_start(args, message);
                    if (DC_LOG_LEVEL >= (int)LogLevel::ERROR)
                    {
                        vlog(LogLevel::ERROR, tag, t, message, args);
                    }
                    va_end(args);
                }

                void warn(
                    const char *tag,
                    std::chrono::time_point<std::chrono::system_clock> t,
                    const char *message,
                    ...)
                {
                    va_list args;
                    va_start(args, message);
                    if (DC_LOG_LEVEL >= (int)LogLevel::WARN)
                    {
                        vlog(LogLevel::WARN, tag, t, message, args);
                    }
                    va_end(args);
                }

                void info(
                    const char *tag,
                    std::chrono::time_point<std::chrono::system_clock> t,
                    const char *message,
                    ...)
                {
                    va_list args;
                    va_start(args, message);
                    if (DC_LOG_LEVEL >= (int)LogLevel::INFO)
                    {
                        vlog(LogLevel::INFO, tag, t, message, args);
                    }
                    va_end(args);
                }

                void debug(
                    const char *tag,
                    std::chrono::time_point<std::chrono::system_clock> t,
                    const char *message,
                    ...)
                {
                    va_list args;
                    va_start(args, message);
                    if (DC_LOG_LEVEL >= (int)LogLevel::DEBUG)
                    {
                        vlog(LogLevel::DEBUG, tag, t, message, args);
                    }
                    va_end(args);
                }

                /**
                 * Triggers any sort of startup behavior that the Logger implementation needs to do its
                 * work, and then should be responsible for starting a worker thread that processes
                 * log messages from any underlying queues
                 * @return
                 */
                virtual bool start() = 0;
                /**
                 * Notifies the Logger implementation that any queued logs should be dumped to output and the
                 * logger should shut itself down
                 */
                virtual void shutdown() = 0;

                /**
                 * Flush the log output from the queue synchronously
                 */
                virtual void flush() = 0;
            };
        }
    }
}

#endif //DEVICE_CLIENT_LOGGER_H
