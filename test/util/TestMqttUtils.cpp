// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/util/MqttUtils.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <iterator>
#include <string>

using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::Util;

static const std::string RESERVED_TOPIC{"$AWS/rules/my-rule/"};

TEST(MqttUtils, TopicValid)
{
    std::string topic{"my-sensor-data"};

    ASSERT_TRUE(MqttUtils::ValidateAwsIotMqttTopicName(topic));
}

TEST(MqttUtils, ReservedTopicValid)
{
    std::string topic(RESERVED_TOPIC);

    ASSERT_TRUE(MqttUtils::ValidateAwsIotMqttTopicName(topic));
}

TEST(MqttUtils, TopicNotValidExceedsMaxLength)
{
    std::string topic(MqttUtils::MAX_LENGTH_OF_TOPIC + 1, 'A');

    ASSERT_FALSE(MqttUtils::ValidateAwsIotMqttTopicName(topic));
}

TEST(MqttUtils, TopicValidExceedsMaxLengthWithReservedTopic)
{
    std::string topic(RESERVED_TOPIC);
    std::fill_n(std::back_inserter(topic), MqttUtils::MAX_LENGTH_OF_TOPIC, 'A');

    ASSERT_TRUE(MqttUtils::ValidateAwsIotMqttTopicName(topic));
}
