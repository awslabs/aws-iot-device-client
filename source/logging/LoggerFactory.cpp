// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "LoggerFactory.h"

using namespace std;
using namespace Aws::Iot::DeviceClient::Logging;

unique_ptr<Logger> fileLogger = unique_ptr<FileLogger>(new FileLogger());
unique_ptr<Logger> stdOutLogger = unique_ptr<StdOutLogger>(new StdOutLogger());

// These assignments allow us to make function calls to perform initial object creation
// for the factory without having to perform that work in the getInstance() method,
// which would happen for every function call
bool initFileLogger = fileLogger->start();
bool initStdOutLogger = stdOutLogger->start();

// Fall back to the StdOutLogger if we can't write to the log file
shared_ptr<Logger> LoggerFactory::logger =
    initFileLogger ? shared_ptr<Logger>(fileLogger.release()) : shared_ptr<Logger>(stdOutLogger.release());

shared_ptr<Logger> LoggerFactory::getLoggerInstance()
{
    return LoggerFactory::logger;
}