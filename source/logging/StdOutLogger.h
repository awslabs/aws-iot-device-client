// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_STDOUTLOGGER_H
#define DEVICE_CLIENT_STDOUTLOGGER_H

#include "Logger.h"
#include "LogLevel.h"
#include "LogQueue.h"

#include <memory>
#include <mutex>

namespace Aws {
    namespace Iot {
        namespace DeviceClient {
            class StdOutLogger final : public Logger {
            private:
                bool needsShutdown = false;
                std::unique_ptr<LogQueue> logQueue = std::unique_ptr<LogQueue>(new LogQueue);

                void run();
                void writeLogMessage(std::unique_ptr<LogMessage> message);
            protected:
                virtual void queueLog(LogLevel level, const char * tag, std::chrono::time_point<std::chrono::system_clock> t, std::string message);
            public:
                virtual bool start();
                virtual void shutdown();
                virtual void flush();
            };
        }
    }
}

#endif //DEVICE_CLIENT_STDOUTLOGGER_H
