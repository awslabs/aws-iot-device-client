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

TEST(StringUtils, maptoString)
{
    Aws::Crt::Map<Aws::Crt::String, Aws::Crt::String> map;
    map.insert(std::pair<Aws::Crt::String, Aws::Crt::String>("a", "b"));
    map.insert(std::pair<Aws::Crt::String, Aws::Crt::String>("c", "d"));
    map.insert(std::pair<Aws::Crt::String, Aws::Crt::String>("e", "f"));
    string expected = "\"a\": \"b\",\n\t"
                      "\"c\": \"d\",\n\t"
                      "\"e\": \"f\"";
    ASSERT_STREQ(expected.c_str(), MapToString(map).c_str());
}

TEST(StringUtils, emptyMaptoString)
{
    Aws::Crt::Map<Aws::Crt::String, Aws::Crt::String> map;
    string expected = "";
    ASSERT_STREQ(expected.c_str(), MapToString(map).c_str());
}

TEST(StringUtils, trimLeftSingleChar)
{
    ASSERT_EQ("a/b/c/", TrimLeftCopy("/a/b/c/", "/")); // Match.
    ASSERT_EQ("a/b/c/", TrimLeftCopy("a/b/c/", "/"));  // No match.
    ASSERT_EQ("", TrimLeftCopy("", "/"));              // Empty string.
}

TEST(StringUtils, trimLeftMultiChar)
{
    ASSERT_EQ("c/", TrimLeftCopy("/a/b/c/", "/ab"));   // Match.
    ASSERT_EQ("/a/b/c", TrimLeftCopy("/a/b/c", "ab")); // No match.
    ASSERT_EQ("", TrimLeftCopy("", "/"));              // Empty string.
}

TEST(StringUtils, trimRightSingleChar)
{
    ASSERT_EQ("/a/b/c", TrimRightCopy("/a/b/c/", "/")); // Match.
    ASSERT_EQ("/a/b/c", TrimRightCopy("/a/b/c", "/"));  // No match.
    ASSERT_EQ("", TrimRightCopy("", "/"));              // Empty string.
}

TEST(StringUtils, trimRightMultiChar)
{
    ASSERT_EQ("/a", TrimRightCopy("/a/b/c/", "/bc"));     // Match.
    ASSERT_EQ("/a/b/c/", TrimRightCopy("/a/b/c/", "bc")); // No match.
    ASSERT_EQ("", TrimRightCopy("", "/"));                // Empty string.
}

TEST(StringUtils, trimSingleChar)
{
    ASSERT_EQ("a/b/c", TrimCopy("/a/b/c/", "/")); // Match.
    ASSERT_EQ("a/b/c", TrimCopy("a/b/c", "/"));   // No match.
    ASSERT_EQ("", TrimCopy("", "/"));             // Empty string.
}

TEST(StringUtils, trimMultiChar)
{
    ASSERT_EQ("b", TrimCopy("/a/b/c/", "/ac"));      // Match.
    ASSERT_EQ("/a/b/c/", TrimCopy("/a/b/c/", "ac")); // No match.
    ASSERT_EQ("", TrimCopy("", "/"));                // Empty string.
}