// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <aws/crt/Types.h>
#include <string>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Util
            {
                /**
                 * \brief Given a message that expects to be formatted with a variable argument list (va_list), this
                 * function will format the message.
                 *
                 * @param message the message to be formatted (The message string must be NULL terminated)
                 * @param args a variable argument list used in the format string
                 * @return a string containing the formatted message
                 */
                std::string vFormatMessage(const char *message, va_list args);

                /**
                 * \brief Given a message that expects to be formatted with additional arguments, this function will
                 * format the message.
                 *
                 * @param message the message to be formatted (The message string must be NULL terminated)
                 * @param ... additional arguments used in the format string
                 * @return a string containing the formatted message
                 */
                std::string FormatMessage(const char *message, ...);

                /**
                 * \brief Given an input string, will sanitize the string to remove dangerous values such as
                 * format specifiers
                 *
                 * @param value the value to sanitize
                 * @return the sanitized value
                 */
                std::string Sanitize(const std::string &value);

                /**
                 * \brief helper method to concat strings with ':' in between
                 *
                 * @param first first string
                 * @param second second string
                 * @return string in 'first:second' form
                 */

                std::string addString(Aws::Crt::String first, Aws::Crt::String second);

                /**
                 * \brief Given an input map object, will return the string form of the map
                 *
                 * @param map the map object to convert into string
                 * @return string form of map
                 */
                std::string MapToString(Crt::Optional<Crt::Map<Aws::Crt::String, Aws::Crt::String>>);

                /**
                 * \brief Return a copy of the string with the leftmost characters from a set removed.
                 * @param s Input string
                 * @param any Set of characters removed
                 * @return string with leftmost characters removed
                 */
                std::string TrimLeftCopy(std::string s, const std::string &any);

                /**
                 * \brief Return a copy of the string with the rightmost characters from a set removed.
                 * @param s Input string
                 * @param any Set of characters removed
                 * @return string with rightmost characters removed
                 */
                std::string TrimRightCopy(std::string s, const std::string &any);

                /**
                 * \brief Return a copy of the string with the leftmost and rightmost characters from a set removed.
                 * @param s Input string
                 * @param any Set of characters removed
                 * @return string with leftmost and rightmost characters removed
                 */
                std::string TrimCopy(std::string s, const std::string &any);
            } // namespace Util
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws
