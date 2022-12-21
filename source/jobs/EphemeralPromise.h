// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_EPHEMERALPROMISE_H
#define DEVICE_CLIENT_EPHEMERALPROMISE_H

#include <future>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Jobs
            {
                /**
                 * \brief Provides the ability to specify a point of expiration for a promise
                 *
                 * This class allows you to specify a point of expiration for a promise indicating that
                 * the promise's time of usefulness is expired. This is used in our std::map of UpdateJobExecution
                 * promises, in case we fail to erase a promise due to an exception or interruption of some kind.
                 * This way, we don't leak the promises.
                 * @tparam T
                 */
                template <typename T> class EphemeralPromise : public std::promise<T>
                {
                  private:
                    /**
                     * \brief The default constructor is hidden as private since it shouldn't be used
                     */
                    EphemeralPromise() = default;
                    /**
                     * \brief The time to live for this promise in milliseconds
                     */
                    std::chrono::milliseconds ttlMillis;
                    /**
                     * \brief The time this promise was created
                     */
                    std::chrono::time_point<std::chrono::system_clock> creationTime;

                  public:
                    explicit EphemeralPromise(std::chrono::milliseconds ttlMillis)
                        : ttlMillis(ttlMillis), creationTime(std::chrono::system_clock::now())
                    {
                    }

                    /**
                     * \brief Whether the EphemeralPromise is expired or not
                     * @return true if expired, false otherwise
                     */
                    bool isExpired() const
                    {
                        return std::chrono::system_clock::now() >= (this->creationTime + this->ttlMillis);
                    }
                };
            } // namespace Jobs
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_EPHEMERALPROMISE_H
