// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "StringUtils.h"
#include <cstdarg>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Util
            {
                std::string vFormatMessage(const char *message, va_list args)
                {
                    va_list copy;
                    va_copy(copy, args);
                    int formattedMsgSize = vsnprintf(nullptr, 0, message, copy) + 1;
                    va_end(copy);

                    std::string buffer;
                    buffer.resize(formattedMsgSize);

                    va_copy(copy, args);
                    vsnprintf(&buffer[0], formattedMsgSize, message, copy);
                    va_end(copy);

                    return buffer;
                }

                std::string FormatMessage(const char *message, ...)
                {
                    va_list args;
                    va_start(args, message);
                    std::string s = vFormatMessage(message, args);
                    va_end(args);
                    return s;
                }
            } // namespace Util
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws
