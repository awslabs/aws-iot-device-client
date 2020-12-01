// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "../../source/util/UniqueString.h"

#include <cctype>

using namespace std;
using namespace Aws::Iot::DeviceClient::Util;

TEST(UniqueString, returnsEmptyString)
{
    string uniqueString = UniqueString::getRandomToken(0);
    ASSERT_STREQ("", uniqueString.c_str());
}

TEST(UniqueString, generatesCorrectLength)
{
    string uniqueString = UniqueString::getRandomToken(10);
    ASSERT_TRUE(uniqueString.size() == 10);
}

TEST(UniqueString, respectsMaxSize)
{
    string uniqueString = UniqueString::getRandomToken(200);
    ASSERT_TRUE(uniqueString.size() == 64);
}

TEST(UniqueString, onlyAlphaNumeric) {
    for(int i = 0; i < 100; i++) {
        string uniqueString = UniqueString::getRandomToken(10);
        for(char& c : uniqueString) {
            ASSERT_TRUE(isalnum(c));
        }
    }
}

TEST(UniqueString, handlesNegativeValues) {
    string uniqueString = UniqueString::getRandomToken(-42);
    ASSERT_TRUE(uniqueString.size() >= 0 && uniqueString.size() <= 64);
}