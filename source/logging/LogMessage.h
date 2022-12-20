// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_LOGMESSAGE_H
#define DEVICE_CLIENT_LOGMESSAGE_H

#include "LogLevel.h"

#include <chrono>
#include <memory>
#include <sstream>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Logging
            {
                /**
                 * \brief Represents all data that a Logger implementation requires to log data, including a LogLevel,
                 * a tag indicating the source of the log message, a time when the message was generated, and the
                 * associated message.
                 */
                class LogMessage
                {
                  private:
                    /**
                     * \brief The LogLevel [DEBUG, INFO, WARN, ERROR]
                     */
                    LogLevel level;
                    /**
                     * \brief A tag used to indicate the source of the log message
                     */
                    std::string tag;
                    /**
                     * \brief The time that the message was logged
                     */
                    std::chrono::time_point<std::chrono::system_clock> time;
                    /**
                     * \brief The message to be logged
                     */
                    std::string message;

                  public:
                    LogMessage(
                        LogLevel level,
                        const std::string &tag,
                        std::chrono::time_point<std::chrono::system_clock> time,
                        const std::string &message)
                        : level(level), tag(tag), time(time), message(message)
                    {
                    }
                    ~LogMessage() = default;

                    /**
                     * \brief Returns the LogLevel of the message
                     * @return the desired LogLevel o fthe message
                     */
                    LogLevel getLevel() const { return level; }
                    /**
                     * \brief Returns the message tag
                     * @return the message tag
                     */
                    std::string getTag() const { return tag; }
                    /**
                     * \brief Returns the time that the message was generated
                     * @return the time that the message was generated
                     */
                    std::chrono::time_point<std::chrono::system_clock> getTime() const { return time; }
                    /**
                     * \brief Returns the log message
                     * @return the log message
                     */
                    std::string &getMessage() { return message; }
                };
            } // namespace Logging
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws
#endif // DEVICE_CLIENT_LOGMESSAGE_H
