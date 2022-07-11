// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/util/ProxyUtils.h"
#include "gtest/gtest.h"

using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::Util;

class ProxyUtilsValidPortNumberParameterizedTestSuite : public ::testing::TestWithParam<int>
{
};

class ProxyUtilsInvalidPortNumberParameterizedTestSuite : public ::testing::TestWithParam<int>
{
};

class ProxyUtilsValidIPAddressParameterizedTestSuite : public ::testing::TestWithParam<std::string>
{
};

class ProxyUtilsInvalidIPAddressParameterizedTestSuite : public ::testing::TestWithParam<std::string>
{
};

TEST_P(ProxyUtilsValidPortNumberParameterizedTestSuite, ValidPortNumberTest)
{
    int portNumber = GetParam();
    ASSERT_TRUE(ProxyUtils::ValidatePortNumber(portNumber));
}

INSTANTIATE_TEST_SUITE_P(
    ProxyUtilsValidPortNumberTest,
    ProxyUtilsValidPortNumberParameterizedTestSuite,
    ::testing::Values(1, 65535));

TEST_P(ProxyUtilsInvalidPortNumberParameterizedTestSuite, InvalidPortNumberTest)
{
    int portNumber = GetParam();
    ASSERT_FALSE(ProxyUtils::ValidatePortNumber(portNumber));
}

INSTANTIATE_TEST_SUITE_P(
    ProxyUtilsValidPortNumberTest,
    ProxyUtilsInvalidPortNumberParameterizedTestSuite,
    ::testing::Values(-1, 65536, -65535, 0));

TEST_P(ProxyUtilsValidIPAddressParameterizedTestSuite, ValidIPAddressTest)
{
    std::string IPAddress = GetParam();
    ASSERT_TRUE(ProxyUtils::ValidateHostIpAddress(IPAddress));
}

INSTANTIATE_TEST_SUITE_P(
    ProxyUtilsValidIPAddressTest,
    ProxyUtilsValidIPAddressParameterizedTestSuite,
    ::testing::Values(
        "10.0.0.0",
        "10.0.0.255",
        "10.255.255.255",
        "172.16.0.0",
        "172.24.0.255",
        "172.31.255.255",
        "192.168.0.0",
        "192.168.1.1",
        "192.168.255.255"));

TEST_P(ProxyUtilsInvalidIPAddressParameterizedTestSuite, ValidIPAddressTest)
{
    std::string IPAddress = GetParam();
    ASSERT_FALSE(ProxyUtils::ValidateHostIpAddress(IPAddress));
}

INSTANTIATE_TEST_SUITE_P(
    ProxyUtilsValidIPAddressTest,
    ProxyUtilsInvalidIPAddressParameterizedTestSuite,
    ::testing::Values(
        "10.0.01",
        "8.8.8.8",
        "amazon.com",
        "9.255.255.255",
        "11.0.0.0",
        "172.15.255.255",
        "172.32.0.0",
        "192.167.255.255",
        "192.169.0.0",
        "255.255.255.255",
        "999.999.999.999"));