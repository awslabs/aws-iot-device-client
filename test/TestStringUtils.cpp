// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../source/util/StringUtils.h"
#include "gtest/gtest.h"

using namespace std;
using namespace Aws::Iot::DeviceClient;

TEST(StringUtils, FormatStringNoArg)
{
    constexpr char expected[] = "Hello world";
    string actual = FormatMessage(expected);
    ASSERT_STREQ(expected, actual.c_str());
}

TEST(StringUtils, FormatStringWithArg)
{
    constexpr char format[] = "I want to eat %d fresh %s.";
    string actual = FormatMessage(format, 1, "apple");
    ASSERT_STREQ("I want to eat 1 fresh apple.", actual.c_str());
}
