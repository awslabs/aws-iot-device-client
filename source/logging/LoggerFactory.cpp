// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "LoggerFactory.h"

using namespace std;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::Logging;

shared_ptr<Logger> LoggerFactory::logger = shared_ptr<Logger>(new StdOutLogger());

shared_ptr<Logger> LoggerFactory::getLoggerInstance()
{
    return LoggerFactory::logger;
}

bool LoggerFactory::reconfigure(const PlainConfig &config)
{
    logger->flush();

    if (config.logConfig.type == config.logConfig.LOG_TYPE_FILE && dynamic_cast<FileLogger *>(logger.get()) == nullptr)
    {
        logger.reset(new FileLogger);
    }
    else if (
        config.logConfig.type == config.logConfig.LOG_TYPE_STDOUT &&
        dynamic_cast<StdOutLogger *>(logger.get()) == nullptr)
    {
        logger.reset(new StdOutLogger);
    }
    return logger->start(config);
}