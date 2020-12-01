// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_JOBENGINE_H
#define DEVICE_CLIENT_JOBENGINE_H

#include <atomic>
#include <string>
#include <sstream>
#include <memory>
#include <condition_variable>
#include <vector>

#include "LimitedStreamBuffer.h"

namespace Aws {
    namespace Iot {
        namespace DeviceClient {
            class JobEngine {
            private:
                const char * TAG = "JobEngine.cpp";

                std::atomic_int errors{0};
                Aws::Iot::DeviceClient::LimitedStreamBuffer stdoutstream;
                Aws::Iot::DeviceClient::LimitedStreamBuffer stderrstream;
            public:
                void processCmdOutput(int fd, bool isStdErr, int childPID);
                int exec_cmd(std::string action, std::vector<std::string> args);
                int hasErrors() { return errors; }
                std::string getStdOut() { return stdoutstream.toString(); };
                std::string getStdErr() { return stderrstream.toString(); };
            };
        }
    }
}


#endif //DEVICE_CLIENT_JOBENGINE_H
