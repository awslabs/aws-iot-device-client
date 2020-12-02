// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "LogLevel.h"

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Logging
            {
                namespace LogLevelMarshaller
                {
                    const char *ToString(LogLevel level)
                    {
                        switch (level)
                        {
                            case LogLevel::ERROR:
                            {
                                return "[ERROR]";
                            }
                            case LogLevel::WARN:
                            {
                                return "[WARN] ";
                            }
                            case LogLevel::INFO:
                            {
                                return "[INFO] ";
                            }
                            case LogLevel::DEBUG:
                            {
                                return "[DEBUG]";
                            }
                            default:
                            {
                                return "";
                            }
                        }
                    }
                } // namespace LogLevelMarshaller
            }     // namespace Logging
        }         // namespace DeviceClient
    }             // namespace Iot
} // namespace Aws
