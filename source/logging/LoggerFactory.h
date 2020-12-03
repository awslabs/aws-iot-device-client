// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_LOGGERFACTORY_H
#define DEVICE_CLIENT_LOGGERFACTORY_H

#define LOG_INFO(tag, message)                                                                                         \
    LoggerFactory::getLoggerInstance().get()->info(tag, std::chrono::system_clock::now(), message)
#define LOG_DEBUG(tag, message)                                                                                        \
    LoggerFactory::getLoggerInstance().get()->debug(tag, std::chrono::system_clock::now(), message)
#define LOG_WARN(tag, message)                                                                                         \
    LoggerFactory::getLoggerInstance().get()->warn(tag, std::chrono::system_clock::now(), message)
#define LOG_ERROR(tag, message)                                                                                        \
    LoggerFactory::getLoggerInstance().get()->error(tag, std::chrono::system_clock::now(), message)

#define LOGM_INFO(tag, message, ...)                                                                                   \
    LoggerFactory::getLoggerInstance().get()->info(tag, std::chrono::system_clock::now(), message, __VA_ARGS__)
#define LOGM_DEBUG(tag, message, ...)                                                                                  \
    LoggerFactory::getLoggerInstance().get()->debug(tag, std::chrono::system_clock::now(), message, __VA_ARGS__)
#define LOGM_WARN(tag, message, ...)                                                                                   \
    LoggerFactory::getLoggerInstance().get()->warn(tag, std::chrono::system_clock::now(), message, __VA_ARGS__)
#define LOGM_ERROR(tag, message, ...)                                                                                  \
    LoggerFactory::getLoggerInstance().get()->error(tag, std::chrono::system_clock::now(), message, __VA_ARGS__)

#include "FileLogger.h"
#include "Logger.h"
#include "StdOutLogger.h"
#include <chrono>
#include <memory>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Logging
            {
                /**
                 * \brief Factory-style class used for instantiation of the logger implementation and access to logging
                 * features
                 *
                 * This class is intended to provide a layer of abstraction between the Device Client and the actual
                 * logger implementation.
                 */
                class LoggerFactory
                {
                  private:
                    /**
                     * \brief The logger implementation
                     */
                    static std::shared_ptr<Logger> logger;

                  public:
                    /**
                     * \brief Returns the active logger instance
                     *
                     * @return an instance of Logger
                     */
                    static std::shared_ptr<Logger> getLoggerInstance();
                };
            } // namespace Logging
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_LOGGERFACTORY_H
