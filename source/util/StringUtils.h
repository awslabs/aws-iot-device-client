// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

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
                 * @param message the message to be formatted
                 * @param args a variable argument list used in the format string
                 * @return a string containing the formatted message
                 */
                std::string vFormatMessage(const char *message, va_list args);

                /**
                 * \brief Given a message that expects to be formatted with additional arguments, this function will
                 * format the message.
                 *
                 * @param message the message to be formatted
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
                std::string Sanitize(std::string value);
            } // namespace Util
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws
