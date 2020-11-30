// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_LIMITEDSTREAMBUFFER_H
#define DEVICE_CLIENT_LIMITEDSTREAMBUFFER_H

#include <string>
#include <deque>
#include <mutex>

namespace Aws {
    namespace Iot {
        namespace DeviceClient {
            class LimitedStreamBuffer {
            private:
                std::mutex bufferLock;
                size_t contentsSize = 0;
                size_t contentsSizeLimit;
                std::deque<std::string> buffer;

            public:
                // Our default content size limit for LimitedStreamBuffer maps to the max allowed number
                // of characters for job status details, since that's the main use of this class
                LimitedStreamBuffer() { this->contentsSizeLimit = 1024; }
                ~LimitedStreamBuffer() = default;
                LimitedStreamBuffer(size_t sizeLimit) { this->contentsSizeLimit = sizeLimit; }

                /**
                 * Add the given string to the LimitedStreamBuffer
                 * @param value a value to add. If the value is larger than the buffer size, it will be trimmed to fit.
                 */
                void addString(std::string value);
                std::string toString();
            };
        }
    }
}

#endif //DEVICE_CLIENT_LIMITEDSTREAMBUFFER_H
