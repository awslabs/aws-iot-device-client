// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/logging/FileLogger.h"
#include "../../source/logging/StdOutLogger.h"
#include "gtest/gtest.h"

using namespace std;
using namespace Aws::Iot::DeviceClient::Logging;

TEST(Logging, swapsLogQueue)
{
    unique_ptr<Logger> stdOutLogger = unique_ptr<Logger>(new StdOutLogger);
    stdOutLogger->error("TAG", std::chrono::system_clock::now(), "Message 1");
    stdOutLogger->error("TAG", std::chrono::system_clock::now(), "Message 2");

    unique_ptr<LogQueue> stdQueue = stdOutLogger->takeLogQueue();
    ASSERT_TRUE(stdQueue->hasNextLog());
    unique_ptr<LogQueue> emptyStdQueue = stdOutLogger->takeLogQueue();
    ASSERT_FALSE(emptyStdQueue->hasNextLog());

    unique_ptr<Logger> fileLogger = unique_ptr<Logger>(new FileLogger);
    fileLogger->setLogQueue(std::move(stdQueue));
    unique_ptr<LogQueue> fileQueue = fileLogger->takeLogQueue();
    ASSERT_TRUE(fileQueue->hasNextLog());
    unique_ptr<LogQueue> emptyFileQueue = fileLogger->takeLogQueue();
}

TEST(Logging, queueNotNullAfterTake)
{
    unique_ptr<Logger> stdOutLogger = unique_ptr<Logger>(new StdOutLogger);
    stdOutLogger->error("TAG", std::chrono::system_clock::now(), "Message 1");
    stdOutLogger->error("TAG", std::chrono::system_clock::now(), "Message 2");

    unique_ptr<LogQueue> stdQueue = stdOutLogger->takeLogQueue();
    ASSERT_TRUE(stdQueue->hasNextLog());

    ASSERT_TRUE(NULL != stdOutLogger->takeLogQueue());
    ASSERT_FALSE(stdOutLogger->takeLogQueue()->hasNextLog());
}
