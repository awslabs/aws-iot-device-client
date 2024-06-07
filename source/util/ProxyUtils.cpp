// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0// Created by Zhang, Bolong on 6/6/22.

#include "ProxyUtils.h"
#include "../logging/LoggerFactory.h"
#include <arpa/inet.h>

using namespace Aws::Iot::DeviceClient::Util;
using namespace Aws::Iot::DeviceClient::Logging;

static constexpr char TAG[] = "ProxyUtils.cpp";

bool ProxyUtils::ValidatePortNumber(const int portNumber)
{
    if (portNumber < 1 || portNumber > UINT16_MAX)
    {
        LOGM_ERROR(TAG, "Port number %u exceeded valid range", portNumber);
        return false;
    }
    return true;
}

bool ProxyUtils::ValidateHostIpAddress(const std::string &ipAddress)
{

    /**
     * inet_pton() was used here to validate the IP address format, it also convert the IP address to a numeric format.
     * AF_INET is the address family that supports IPv4,
     * the in_addr structure will be populated with the address information.
     *
     * It returns 1 on success, 0 or -1 for varies of reasons of invalid conversion.
     */
    struct in_addr addr;
    // cppcheck-suppress variableScope
    uint32_t IPv4Identifier;
    int result = inet_pton(AF_INET, ipAddress.c_str(), &addr);

    if (result > 0)
    {
        IPv4Identifier = ntohl(*((uint32_t *)&addr));
        return IsIpAddressPrivate(IPv4Identifier);
    }
    return false;
}

bool ProxyUtils::IsIpAddressPrivate(std::uint32_t ipAddress)
{
    return (ipAddress >= DECIMAL_REP_IP_10_0_0_0 && ipAddress <= DECIMAL_REP_IP_10_255_255_255) ||
           (ipAddress >= DECIMAL_REP_IP_172_16_0_0 && ipAddress <= DECIMAL_REP_IP_172_31_255_255) ||
           (ipAddress >= DECIMAL_REP_IP_192_168_0_0 && ipAddress <= DECIMAL_REP_IP_192_168_255_255);
}
