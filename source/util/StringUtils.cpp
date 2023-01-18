// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "StringUtils.h"
#include "../config/Config.h"
#include <cstdarg>
#include <map>
#include <regex>

using namespace std;

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Util
            {
                string vFormatMessage(const char message[], va_list args)
                {
                    va_list copy;
                    va_copy(copy, args);
                    size_t formattedMsgSize = vsnprintf(nullptr, 0, message, copy);
                    va_end(copy);

                    // will format up to MAX_CONFIG_SIZE, the maximum buffer size after which log messages will get
                    // truncated
                    formattedMsgSize = std::min(formattedMsgSize, Config::MAX_CONFIG_SIZE - 1) + 1;

                    std::unique_ptr<char[]> buffer(new char[formattedMsgSize]);

                    va_copy(copy, args);
                    vsnprintf(buffer.get(), formattedMsgSize, message, copy);
                    va_end(copy);

                    return string(buffer.get(), buffer.get() + formattedMsgSize - 1);
                }

                string FormatMessage(const char message[], ...)
                {
                    va_list args;
                    va_start(args, message);
                    //  The message string used over here must be NULL terminated
                    std::string s = vFormatMessage(message, args);
                    va_end(args);
                    return s;
                }

                string Sanitize(const std::string &value)
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

                string addString(const Aws::Crt::String &first, const Aws::Crt::String &second)
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

                // cppcheck-suppress unusedFunction
                string TrimLeftCopy(string s, const string &any) { return s.erase(0, s.find_first_not_of(any)); }

                string TrimRightCopy(string s, const string &any) { return s.erase(s.find_last_not_of(any) + 1); }

                string TrimCopy(string s, const string &any)
                {
                    s.erase(0, s.find_first_not_of(any));
                    s.erase(s.find_last_not_of(any) + 1);
                    return s;
                }

                vector<string> ParseToVectorString(const JsonView &json)
                {
                    vector<string> plainVector;

                    for (const auto &i : json.AsArray())
                    {
                        // cppcheck-suppress useStlAlgorithm
                        plainVector.push_back(i.AsString().c_str());
                    }
                    return plainVector;
                }

                vector<string> SplitStringByComma(const string &stringToSplit)
                {
                    regex delim{R"((\\,|[^,])+)"};

                    vector<string> tokens;
                    copy(
                        sregex_token_iterator{begin(stringToSplit), end(stringToSplit), delim},
                        sregex_token_iterator{},
                        back_inserter(tokens));
                    return tokens;
                }

                void replace_all(string &inout, const string &what, const string &with)
                {
                    for (string::size_type pos{}; inout.npos != (pos = inout.find(what.data(), pos, what.length()));
                         pos += with.length())
                    {
                        inout.replace(pos, what.length(), with.data(), with.length());
                    }
                }
            } // namespace Util
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws
