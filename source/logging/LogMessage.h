// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_LOGMESSAGE_H
#define DEVICE_CLIENT_LOGMESSAGE_H

#include "LogLevel.h"

#include <chrono>
#include <memory>
#include <sstream>

namespace Aws {
    namespace Iot {
        namespace DeviceClient {
            class LogMessage {
            private:
                LogLevel level;
                std::string tag;
                std::chrono::time_point<std::chrono::system_clock> time;
                std::string message;
            public:
                LogMessage(LogLevel level, std::string tag, std::chrono::time_point<std::chrono::system_clock> time, std::string message) {
                    this->level = level;
                    this->tag = tag;
                    this->time = time;
                    this->message = message;
                }
                ~LogMessage() = default;

                LogLevel getLevel() { return level; }
                std::string getTag() { return tag; }
                std::chrono::time_point<std::chrono::system_clock> getTime() { return time; }
                std::string& getMessage() { return message; }

            };
        }
    }
}
#endif //DEVICE_CLIENT_LOGMESSAGE_H
