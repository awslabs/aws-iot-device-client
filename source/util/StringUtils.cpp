// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "StringUtils.h"
#include <cstdarg>
#include <map>
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
                    //  The message string used over here must be NULL terminated
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

                string addString(Aws::Crt::String first, Aws::Crt::String second)
                {
                    string jsonTemplate = R"("%s": "%s")";
                    //  The message string used over here must be NULL terminated
                    return FormatMessage(jsonTemplate.c_str(), first.c_str(), second.c_str());
                }

                string MapToString(Crt::Optional<Crt::Map<Aws::Crt::String, Aws::Crt::String>> map)
                {
                    std::map<Aws::Crt::String, Aws::Crt::String>::iterator it;
                    string result = "";
                    unsigned int count = 0;
                    for (it = map->begin(); it != map->end(); ++it)
                    {
                        count++;
                        result = result.append(addString(it->first, it->second));
                        if (count != map->size())
                        {
                            result = result.append(",\n\t");
                        }
                    }
                    result.erase(remove(result.begin(), result.end(), '\0'), result.end());
                    return result;
                }

                string TrimLeftCopy(string s, const string &any) { return s.erase(0, s.find_first_not_of(any)); }

                string TrimRightCopy(string s, const string &any) { return s.erase(s.find_last_not_of(any) + 1); }

                string TrimCopy(string s, const string &any)
                {
                    s.erase(0, s.find_first_not_of(any));
                    s.erase(s.find_last_not_of(any) + 1);
                    return s;
                }
            } // namespace Util
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws
