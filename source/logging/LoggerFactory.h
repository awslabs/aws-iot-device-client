// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_LOGGERFACTORY_H
#define DEVICE_CLIENT_LOGGERFACTORY_H

/**
 * \brief Log INFO message
 *
 * @param tag the tag to be attached with the log message (The tag string must be NULL terminated)
 * @param message the information message to be logged (The message string must be NULL terminated)
 */
#define LOG_INFO(tag, message)                                                                                         \
    LoggerFactory::getLoggerInstance().get()->info(tag, std::chrono::system_clock::now(), message)
/**
 * \brief Log DEBUG message
 *
 * @param tag the tag to be attached with the log message (The tag string must be NULL terminated)
 * @param message the debug message to be logged (The message string must be NULL terminated)
 */
#define LOG_DEBUG(tag, message)                                                                                        \
    LoggerFactory::getLoggerInstance().get()->debug(tag, std::chrono::system_clock::now(), message)
/**
 * \brief Log WARN message
 *
 * @param tag the tag to be attached with the log message (The tag string must be NULL terminated)
 * @param message the warning message to be logged (The message string must be NULL terminated)
 */
#define LOG_WARN(tag, message)                                                                                         \
    LoggerFactory::getLoggerInstance().get()->warn(tag, std::chrono::system_clock::now(), message)
/**
 * \brief Log ERROR message
 *
 * @param tag the tag to be attached with the log message (The tag string must be NULL terminated)
 * @param message the error message to be logged (The message string must be NULL terminated)
 */
#define LOG_ERROR(tag, message)                                                                                        \
    LoggerFactory::getLoggerInstance().get()->error(tag, std::chrono::system_clock::now(), message)

/**
 * \brief Log INFO message
 *
 * @param tag the tag to be attached with the log message (The tag string must be NULL terminated)
 * @param message the information message to be logged (The message string must be NULL terminated)
 * @param ... additional arguments used in the format string
 */
#define LOGM_INFO(tag, message, ...)                                                                                   \
    LoggerFactory::getLoggerInstance().get()->info(tag, std::chrono::system_clock::now(), message, __VA_ARGS__)
/**
 * \brief Log DEBUG message
 *
 * @param tag the tag to be attached with the log message (The tag string must be NULL terminated)
 * @param message the debug message to be logged (The message string must be NULL terminated)
 * @param ... additional arguments used in the format string
 */
#define LOGM_DEBUG(tag, message, ...)                                                                                  \
    LoggerFactory::getLoggerInstance().get()->debug(tag, std::chrono::system_clock::now(), message, __VA_ARGS__)
/**
 * \brief Log WARN message
 *
 * @param tag the tag to be attached with the log message (The tag string must be NULL terminated)
 * @param message the warning message to be logged (The message string must be NULL terminated)
 * @param ... additional arguments used in the format string
 */
#define LOGM_WARN(tag, message, ...)                                                                                   \
    LoggerFactory::getLoggerInstance().get()->warn(tag, std::chrono::system_clock::now(), message, __VA_ARGS__)
/**
 * \brief Log ERROR message
 *
 * @param tag the tag to be attached with the log message (The tag string must be NULL terminated)
 * @param message the error message to be logged (The message string must be NULL terminated)
 * @param ... additional arguments used in the format string
 */
#define LOGM_ERROR(tag, message, ...)                                                                                  \
    LoggerFactory::getLoggerInstance().get()->error(tag, std::chrono::system_clock::now(), message, __VA_ARGS__)

#include "../config/Config.h"
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
                    static constexpr char TAG[] = "LoggerFactory.cpp";
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

                    /**
                     * \brief Reconfigure the logger to use a new set of settings. This may include changing the
                     * log level or switching between logger implementations.
                     *
                     * @param config
                     * @return
                     */
                    static bool reconfigure(const PlainConfig &config);
                };
            } // namespace Logging
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_LOGGERFACTORY_H
