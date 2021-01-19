// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/util/StringUtils.h"
#include "gtest/gtest.h"

using namespace std;
using namespace Aws::Iot::DeviceClient::Util;

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

TEST(StringUtils, sanitizeRemovesFormatSpecifier)
{
    string original = "abc123 %s";
    ASSERT_STREQ("abc123  s", Sanitize(original).c_str());
}

TEST(StringUtils, sanitizeRemovesMultipleFormatSpecifiers)
{
    string original = "%s %zu %d %s";
    ASSERT_STREQ(" s  zu  d  s", Sanitize(original).c_str());
}

TEST(StringUtils, sanitizeLeavesAcceptableCharactersAlone)
{
    string original = "~!@#$^&*()_+`1234567890-={}|[]\\:'<>?,./'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    ASSERT_STREQ(original.c_str(), Sanitize(original).c_str());
}

TEST(StringUtils, sanitizeHandlesEmptyString)
{
    string original = "";
    ASSERT_STREQ(original.c_str(), Sanitize(original).c_str());
}

TEST(StringUtils, leavesNewLineAndTabAlone)
{
    string original = "\toriginal\n";
    ASSERT_STREQ(original.c_str(), Sanitize(original).c_str());
}