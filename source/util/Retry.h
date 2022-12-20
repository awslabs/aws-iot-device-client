// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_RETRY_H
#define DEVICE_CLIENT_RETRY_H

#include <atomic>
#include <functional>
#include <future>
#include <thread>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Util
            {
                /**
                 * \brief Provides utility methods for retrying a function
                 */
                class Retry
                {
                  private:
                    static const char *TAG;

                  public:
                    /**
                     * \brief Used for passing an exponential retry configuration to the exponentialBackoff function
                     */
                    struct ExponentialRetryConfig
                    {
                        /**
                         * \brief The initial amount of time between retries in milliseconds
                         */
                        long startingBackoffMillis;
                        /**
                         * \brief The maximum amount of time between retries in milliseconds
                         */
                        long maxBackoffMillis;
                        /**
                         * \brief The maximum number of retries to perform. *NOTE* if the specified number is
                         * negative, the exponentialBackoff function will retry the provided function an infinite
                         * number of times until it returns successfully or is shut down.
                         */
                        long maxRetries;

                        std::atomic<bool> *needStopFlag;
                    };
                    /**
                     * \brief Performs an exponential backoff of the provided function based on the specified
                     * ExponentialRetryConfig
                     *
                     * In the event of throttling by IoT Core APIs, such as when we perform UpdateJobExecution
                     * within the Jobs feature, it is necessary to perform an exponential backoff to improve the
                     * chances of receiving a success response.
                     * @param config the ExponentialRetryConfig specifying whether the function should be retried
                     * @param retryableFunction the function to retry. This function should return a bool indicating
                     * whether it is successful or not, since this indicator is what will determine whether the
                     * function is retried or not.
                     * @param onComplete a callback function which will be executed once this function is finished
                     * attempting retries
                     * @return a bool representing whether the retryableFunction was successful or not
                     */
                    static bool exponentialBackoff(
                        const ExponentialRetryConfig &config,
                        const std::function<bool()> &retryableFunction,
                        const std::function<void()> &onComplete = nullptr);
                };
            } // namespace Util
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_RETRY_H
