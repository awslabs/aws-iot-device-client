// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/jobs/LimitedStreamBuffer.h"
#include "gtest/gtest.h"

using namespace std;
using namespace Aws::Iot::DeviceClient::Jobs;

TEST(LimitedStreamBuffer, returnsEmptyString)
{
    LimitedStreamBuffer buffer;
    ASSERT_STREQ("", buffer.toString().c_str());
}

TEST(LimitedStreamBuffer, acceptsValues)
{
    LimitedStreamBuffer buffer(10);
    buffer.addString("one");
    buffer.addString("two");

    ASSERT_STREQ("onetwo", buffer.toString().c_str());
}

TEST(LimitedStreamBuffer, evictsFront)
{
    LimitedStreamBuffer buffer(10);
    buffer.addString("one");
    buffer.addString("two");
    buffer.addString("three");

    ASSERT_STREQ("twothree", buffer.toString().c_str());
}

TEST(LimitedStreamBuffer, removesExistingEntries)
{
    LimitedStreamBuffer buffer(11);

    buffer.addString("one");
    buffer.addString("two");
    buffer.addString("three");

    ASSERT_STREQ("onetwothree", buffer.toString().c_str());
    buffer.addString("elevenChars");
    ASSERT_STREQ("elevenChars", buffer.toString().c_str());

    // Test one more time to make sure the buffer still holds the value
    ASSERT_STREQ("elevenChars", buffer.toString().c_str());
}

TEST(LimitedStreamBuffer, clipsLengthyEntry)
{
    LimitedStreamBuffer buffer(5);
    buffer.addString("seventeen");
    ASSERT_STREQ("nteen", buffer.toString().c_str());
}

TEST(LimitedStreamBuffer, allowsSameLengthEntry)
{
    LimitedStreamBuffer buffer(9);
    buffer.addString("testentry");
    ASSERT_STREQ("testentry", buffer.toString().c_str());
}
