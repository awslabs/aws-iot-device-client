// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "LoggerFactory.h"

using namespace std;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::Logging;

shared_ptr<Logger> LoggerFactory::logger = std::make_shared<StdOutLogger>();

shared_ptr<Logger> LoggerFactory::getLoggerInstance()
{
    return LoggerFactory::logger;
}

bool LoggerFactory::reconfigure(const PlainConfig &config)
{
    if (config.logConfig.deviceClientLogtype == PlainConfig::LogConfig::LOG_TYPE_FILE &&
        dynamic_cast<FileLogger *>(logger.get()) == nullptr)
    {
        logger->stop();
        unique_ptr<LogQueue> logQueue = logger->takeLogQueue();
        logger.reset(new FileLogger);
        logger->setLogQueue(std::move(logQueue));
    }
    else if (
        config.logConfig.deviceClientLogtype == PlainConfig::LogConfig::LOG_TYPE_STDOUT &&
        dynamic_cast<StdOutLogger *>(logger.get()) == nullptr)
    {
        logger->stop();
        unique_ptr<LogQueue> logQueue = logger->takeLogQueue();
        logger.reset(new StdOutLogger);
        logger->setLogQueue(std::move(logQueue));
    }
    return logger->start(config);
}