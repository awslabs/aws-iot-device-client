// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_UNIQUESTRING_H
#define DEVICE_CLIENT_UNIQUESTRING_H

#include <cstddef>
#include <string>

namespace Aws {
    namespace Iot {
        namespace DeviceClient {
            namespace Util {
                class UniqueString {
                public:
                    const static size_t MAX_CLIENT_TOKEN_SIZE = 64;

                    static std::string getRandomToken(size_t length);
                };
            }
        }
    }
}

#endif //DEVICE_CLIENT_UNIQUESTRING_H
