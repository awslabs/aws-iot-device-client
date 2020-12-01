// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_RETRY_H
#define DEVICE_CLIENT_RETRY_H

#include <atomic>
#include <functional>
#include <future>

struct RetryConfig {
    long startingBackoffMillis;
    long maxBackoffMillis;
    long maxRetries;
    std::mutex& stopMutex;
    bool& needStopFlag;
};

namespace Aws {
    namespace Iot {
        namespace DeviceClient {
            class Retry {
            private:
                static const char * TAG;
            public:
                static bool exponentialBackoff(std::function<bool()> retryableFunction, RetryConfig config);
            };
        }
    }
}

#endif //DEVICE_CLIENT_RETRY_H
