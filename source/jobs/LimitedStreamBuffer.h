// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_LIMITEDSTREAMBUFFER_H
#define DEVICE_CLIENT_LIMITEDSTREAMBUFFER_H

#include <deque>
#include <mutex>
#include <string>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Jobs
            {
                /** \brief Used to buffer output from STDOUT or STDERR of the child process for placement
                 * in the status details when updating a job execution.
                 */
                class LimitedStreamBuffer
                {
                  private:
                    /**
                     * \brief Used to improve the thread safety of this class by preventing concurrent reads and writes
                     */
                    std::mutex bufferLock;
                    /**
                     * \brief The current size of the buffer
                     */
                    size_t contentsSize = 0;
                    /**
                     * \brief The maximum allowable size of this buffer
                     */
                    size_t contentsSizeLimit;
                    /**
                     * \brief The underlying deque implementation used to buffer words
                     */
                    std::deque<std::string> buffer;

                  public:
                    // Our default content size limit for LimitedStreamBuffer maps to the max allowed number
                    // of characters for job status details, since that's the main use of this class
                    LimitedStreamBuffer() : contentsSizeLimit(1024) {}

                    ~LimitedStreamBuffer() = default;

                    /**
                     * \brief We provide an additional constructor with a configurable sizeLimit for testing
                     * @param sizeLimit the maximum size of the LimitedStreamBuffer
                     */
                    explicit LimitedStreamBuffer(size_t sizeLimit) : contentsSizeLimit(sizeLimit) {}

                    /**
                     * \brief Add the given string to the LimitedStreamBuffer
                     * @param value the value to add
                     */
                    void addString(const std::string &value);

                    /**
                     * \brief Generates a string value from the contents of the buffer
                     * @return a string representing the combined contents of the LimitedStreamBuffer
                     */
                    std::string toString();
                };
            } // namespace Jobs
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_LIMITEDSTREAMBUFFER_H
