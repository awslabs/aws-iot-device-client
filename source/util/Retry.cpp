// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "Retry.h"
#include "../logging/LoggerFactory.h"
#include <thread>

using namespace std;
using namespace Aws::Iot::DeviceClient::Logging;
using namespace Aws::Iot::DeviceClient::Util;

const char *Retry::TAG = "Retry.cpp";

bool Retry::exponentialBackoff(
    const ExponentialRetryConfig &config,
    const function<bool()> &retryableFunction,
    const function<void()> &onComplete)
{
    bool needToStop = false;
    if (config.needStopFlag != nullptr)
    {
        needToStop = config.needStopFlag->load();
    }

    if (needToStop)
    {
        LOG_DEBUG(TAG, "Stop flag was set prior to executing retryable function, will not attempt retryable execution");

        if (nullptr != onComplete)
        {
            onComplete();
        }

        return false;
    }

    if (config.maxRetries < 0)
    {
        LOG_DEBUG(TAG, "Retryable function starting, it will retry until success");
    }

    bool successful = false;
    long backoffMillis = config.startingBackoffMillis;
    long retriesSoFar = 0;
    while (!successful && !needToStop && (config.maxRetries < 0 || retriesSoFar < config.maxRetries))
    {
        successful = retryableFunction();

        // So we don't have to worry about overflowing on an infinite number of retries
        if (config.maxRetries >= 0)
        {
            retriesSoFar++;
        }

        if (!successful && (config.maxRetries < 0 || retriesSoFar < config.maxRetries))
        {
            LOGM_DEBUG(TAG, "Retryable function returned unsuccessfully, sleeping for %ld milliseconds", backoffMillis);
            this_thread::sleep_for(std::chrono::milliseconds(backoffMillis));
            backoffMillis = backoffMillis * 2 > config.maxBackoffMillis ? config.maxBackoffMillis : backoffMillis * 2;
        }

        if (config.needStopFlag != nullptr)
        {
            needToStop = config.needStopFlag->load();
        }
    }

    if (nullptr != onComplete)
    {
        onComplete();
    }

    return successful;
}