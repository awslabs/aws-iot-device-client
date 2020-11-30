// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_FILELOGGER_H
#define DEVICE_CLIENT_FILELOGGER_H

#include <mutex>
#include <stdio.h>
#include <memory>
#include <fstream>

#include "Logger.h"
#include "LogLevel.h"
#include "LogQueue.h"

namespace Aws {
    namespace Iot {
        namespace DeviceClient {
            class FileLogger final : public Logger {
            private:
                std::string DEFAULT_LOG_DIR = "/var/log/";
                std::string DEFAULT_LOG_FILE = "/var/log/aws-iot-device-client.log";

                bool needsShutdown = false;
                std::unique_ptr<LogQueue> logQueue = std::unique_ptr<LogQueue>(new LogQueue);
                std::unique_ptr<std::ofstream> outputStream;

                static int mkdirs(const char * path);
                void writeLogMessage(std::unique_ptr<LogMessage> message);
                virtual void queueLog(LogLevel level, const char * tag, std::chrono::time_point<std::chrono::system_clock> t, std::string message);
                void createLogDirectories();
                void run();
            public:
                virtual bool start();
                virtual void shutdown();
                virtual void flush();
            };
        }
    }
}

#endif //DEVICE_CLIENT_FILELOGGER_H
