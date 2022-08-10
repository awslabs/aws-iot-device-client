// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/jobs/EphemeralPromise.h"
#include "gtest/gtest.h"
#include <thread>

using namespace std;
using namespace Aws::Iot::DeviceClient::Jobs;

TEST(EphemeralPromise, promiseExpires)
{
    EphemeralPromise<int> promise(std::chrono::milliseconds(10));
    ASSERT_FALSE(promise.isExpired());

    this_thread::sleep_for(std::chrono::milliseconds(25));
    ASSERT_TRUE(promise.isExpired());
}

TEST(EphemeralPromise, standardPromiseFeaturesWork)
{
    EphemeralPromise<int> promise(std::chrono::milliseconds(10));
    promise.set_value(5);
    ASSERT_EQ(5, promise.get_future().get());
}
