// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_FEATURE_H
#define DEVICE_CLIENT_FEATURE_H

#include <string>

namespace Aws {
    namespace Iot {
        namespace DeviceClient {
            class Feature {
                public:
                    virtual ~Feature() {}
                
                    virtual std::string get_name() = 0;
                    virtual int start() = 0;
                    virtual int stop() = 0;
            };
        }
    }
}

#endif //DEVICE_CLIENT_FEATURE_H