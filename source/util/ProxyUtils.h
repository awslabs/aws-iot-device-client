// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include <cstdint>
#include <string>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Util
            {
                namespace ProxyUtils
                {
                    static constexpr uint32_t DECIMAL_REP_IP_10_0_0_0 = 167772160;
                    static constexpr uint32_t DECIMAL_REP_IP_10_255_255_255 = 184549375;
                    static constexpr uint32_t DECIMAL_REP_IP_172_16_0_0 = 2886729728;
                    static constexpr uint32_t DECIMAL_REP_IP_172_31_255_255 = 2887778303;
                    static constexpr uint32_t DECIMAL_REP_IP_192_168_0_0 = 3232235520;
                    static constexpr uint32_t DECIMAL_REP_IP_192_168_255_255 = 3232301055;
                    /**
                     * \brief Validates port number is in the valid range
                     *
                     * @param port port number
                     * @return returns true if port number is in the valid range
                     */
                    bool ValidatePortNumber(int portNumber);

                    /**
                     * \brief Validates IP address is in the reserved private block range
                     *
                     * @param ipAddress IP address
                     * @return returns true if IP is in the valid range
                     */
                    bool ValidateHostIpAddress(const std::string &ipAddress);

                    bool IsIpAddressPrivate(std::uint32_t ipAddress);
                } // namespace ProxyUtils
            }     // namespace Util
        }         // namespace DeviceClient
    }             // namespace Iot
} // namespace Aws
