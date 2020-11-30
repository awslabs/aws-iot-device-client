// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_LOGGERFACTORY_H
#define DEVICE_CLIENT_LOGGERFACTORY_H

#define LOG_INFO(tag, message) LoggerFactory::getLoggerInstance().get()->info(tag, std::chrono::system_clock::now(), message)
#define LOG_DEBUG(tag, message) LoggerFactory::getLoggerInstance().get()->debug(tag, std::chrono::system_clock::now(), message)
#define LOG_WARN(tag, message) LoggerFactory::getLoggerInstance().get()->warn(tag, std::chrono::system_clock::now(), message)
#define LOG_ERROR(tag, message) LoggerFactory::getLoggerInstance().get()->error(tag, std::chrono::system_clock::now(), message)

#define LOGM_INFO(tag, message, ...) LoggerFactory::getLoggerInstance().get()->info(tag, std::chrono::system_clock::now(), message, __VA_ARGS__)
#define LOGM_DEBUG(tag, message, ...) LoggerFactory::getLoggerInstance().get()->debug(tag, std::chrono::system_clock::now(), message, __VA_ARGS__)
#define LOGM_WARN(tag, message, ...) LoggerFactory::getLoggerInstance().get()->warn(tag, std::chrono::system_clock::now(), message, __VA_ARGS__)
#define LOGM_ERROR(tag, message, ...) LoggerFactory::getLoggerInstance().get()->error(tag, std::chrono::system_clock::now(), message, __VA_ARGS__)

#include <memory>
#include <chrono>
#include "Logger.h"
#include "FileLogger.h"
#include "StdOutLogger.h"

namespace Aws {
    namespace Iot {
        namespace DeviceClient {
            class LoggerFactory {
            private:
                static std::shared_ptr<Logger> logger;
            public:
                static std::shared_ptr<Logger> getLoggerInstance();
            };
        }
    }
}


#endif //DEVICE_CLIENT_LOGGERFACTORY_H
