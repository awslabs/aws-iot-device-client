// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "Retry.h"
#include "../logging/LoggerFactory.h"

using namespace std;
using namespace Aws::Iot::DeviceClient;

const char * Retry::TAG = "Retry.cpp";

bool Retry::exponentialBackoff(function<bool()> retryableFunction, RetryConfig config) {
    if(config.maxRetries < 0) {
        LOG_DEBUG(TAG, "Retryable function will retry until success");
    }

    bool successful = false;
    unique_lock<mutex> needStopLock(config.stopMutex);
    bool needToStop = config.needStopFlag;
    needStopLock.unlock();
    long backoffMillis = config.startingBackoffMillis;
    long retriesSoFar = 0;
    while(!successful && !needToStop && (config.maxRetries < 0 || retriesSoFar <= config.maxRetries)) {
        successful = retryableFunction();
        // So we don't have to worry about overflowing on an infinite number of retries
        if(config.maxRetries >= 0) {
            retriesSoFar++;
        }

        if(!successful) {
            LOGM_DEBUG(TAG, "Retryable function returned unsuccessfully, sleeping for %ld milliseconds", backoffMillis);
            this_thread::sleep_for(std::chrono::milliseconds (backoffMillis));
            backoffMillis = backoffMillis * 2 > config.maxBackoffMillis ? config.maxBackoffMillis : backoffMillis * 2;
        }

        unique_lock<mutex>(config.stopMutex);
        needToStop = config.needStopFlag;
    }

    return successful;
}
