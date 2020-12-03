// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_LOGLEVEL_H
#define DEVICE_CLIENT_LOGLEVEL_H

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Logging
            {
                enum class LogLevel
                {
                    ERROR = 0,
                    WARN = 1,
                    INFO = 2,
                    DEBUG = 3
                };

                namespace LogLevelMarshaller
                {
                    const char *ToString(LogLevel level);
                }
            } // namespace Logging
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_LOGLEVEL_H
