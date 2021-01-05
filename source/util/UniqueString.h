// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_UNIQUESTRING_H
#define DEVICE_CLIENT_UNIQUESTRING_H

#include <cstddef>
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
                 * \brief Utility class intended to provide a somewhat 'unique' token.
                 *
                 * We do not make any promises about the uniqueness of the generated token, only that
                 * it is hopefully unique enough for our purposes. IE we currently use the getRandomToken
                 * function to generate a token that can be used to map UpdateJobExecution requests back to their
                 * responses (in which only a few should be in flight at any given time) but this function
                 * would not be ideal for keys across a large store of data.
                 */
                class UniqueString
                {
                  public:
                    const static size_t MAX_CLIENT_TOKEN_SIZE = 64;

                    static std::string GetRandomToken(size_t length);
                };
            } // namespace Util
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_UNIQUESTRING_H
