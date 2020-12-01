// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "LogLevel.h"

namespace Aws {
    namespace Iot {
        namespace DeviceClient {
            namespace LogLevelMarshaller {
                const char * ToString(LogLevel level) {
                    switch(level) {
                        case LogLevel::ERROR: {
                            return "[ERROR]";
                        }
                        case LogLevel::WARN: {
                            return "[WARN] ";
                        }
                        case LogLevel::INFO: {
                            return "[INFO] ";
                        }
                        case LogLevel::DEBUG: {
                            return "[DEBUG]";
                        }
                        default: {
                            return "";
                        }
                    }
                }
            }
        }
    }
}

