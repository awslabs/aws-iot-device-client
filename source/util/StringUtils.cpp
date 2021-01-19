// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "StringUtils.h"
#include <cstdarg>
#include <sstream>

using namespace std;

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Util
            {
                string vFormatMessage(const char *message, va_list args)
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

                string FormatMessage(const char *message, ...)
                {
                    va_list args;
                    va_start(args, message);
                    std::string s = vFormatMessage(message, args);
                    va_end(args);
                    return s;
                }

                string Sanitize(std::string value)
                {
                    const char *input = value.c_str();
                    ostringstream output;
                    for (size_t i = 0; i < value.length(); i++)
                    {
                        char c = input[i];
                        if ((9 <= c && c <= 10) // Tab and NewLine control characters
                            || (32 <= c && c <= 36) || (38 <= c && c <= 126))
                        {
                            output << c;
                        }
                        else
                        {
                            output << ' ';
                        }
                    }
                    return output.str();
                }
            } // namespace Util
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws
