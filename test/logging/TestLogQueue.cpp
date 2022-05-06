// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/logging/LogQueue.h"
#include "gtest/gtest.h"

#include <thread>

using namespace std;
using namespace Aws::Iot::DeviceClient::Logging;

class LogQueueTest : public ::testing::Test
{
  protected:
    unique_ptr<LogQueue> logQueue;

    condition_variable cv;
    condition_variable cv2;
    mutex m;
    mutex m2;
    bool processed = false;
    bool processed2 = false;

    void SetUp() override
    {
        logQueue = unique_ptr<LogQueue>(new LogQueue);
        logQueue->addLog(unique_ptr<LogMessage>(
            new LogMessage(LogLevel::DEBUG, "TAG", std::chrono::system_clock::now(), "Message 1")));
        logQueue->addLog(unique_ptr<LogMessage>(
            new LogMessage(LogLevel::DEBUG, "TAG", std::chrono::system_clock::now(), "Message 2")));
    }

  public:
    void processMessages(bool firstLock)
    {
        unique_ptr<LogMessage> message = logQueue->getNextLog();

        while (NULL != message)
        {
            message = logQueue->getNextLog();
        }

        if (firstLock)
        {
            unique_lock<mutex>{m};
            processed = true;
            cv.notify_all();
        }
        else
        {
            unique_lock<mutex>{m2};
            processed2 = true;
            cv2.notify_all();
        }
    }
};

TEST_F(LogQueueTest, queuesMessages)
{
    ASSERT_TRUE(logQueue->hasNextLog());
    ASSERT_STREQ("Message 1", logQueue->getNextLog()->getMessage().c_str());
    ASSERT_STREQ("Message 2", logQueue->getNextLog()->getMessage().c_str());
}

TEST_F(LogQueueTest, removesMessagesFromQueue)
{
    for (int i = 0; i < 2; i++)
    {
        logQueue->getNextLog();
    }
    ASSERT_FALSE(logQueue->hasNextLog());
}

TEST_F(LogQueueTest, notifyAllOnShutdown)
{
    // We need to load some additional messages into the log queue first
    unique_lock<mutex> lock1 = unique_lock<mutex>(m);
    unique_lock<mutex> lock2 = unique_lock<mutex>(m2);

    thread thread1(&LogQueueTest::processMessages, this, true);
    thread thread2(&LogQueueTest::processMessages, this, false);

    thread1.detach();
    thread2.detach();

    logQueue->shutdown();

    // 400ms is 2 * EMPTY_WAIT_TIME_MILLISECONDS as defined in LogQueue.h
    cv.wait_for(lock1, chrono::milliseconds(400));
    cv2.wait_for(lock2, chrono::milliseconds(400));

    ASSERT_TRUE(processed && processed2);
}

TEST_F(LogQueueTest, readsAllMessagesWithNullAtBeginning)
{
    logQueue->addLog(NULL);
    logQueue->addLog(
        unique_ptr<LogMessage>(new LogMessage(LogLevel::DEBUG, "TAG", std::chrono::system_clock::now(), "Message")));
    logQueue->addLog(
        unique_ptr<LogMessage>(new LogMessage(LogLevel::DEBUG, "TAG", std::chrono::system_clock::now(), "Message")));

    int counter = 0;
    while (logQueue->hasNextLog())
    {
        logQueue->getNextLog();
        counter++;
    }

    ASSERT_EQ(5, counter);
}
