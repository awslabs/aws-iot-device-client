// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "MqttUtils.h"

#include "../logging/LoggerFactory.h"

#include <algorithm>
#include <regex>

using namespace Aws::Iot::DeviceClient::Util;
using namespace Aws::Iot::DeviceClient::Logging;

static constexpr char TAG[] = "MqttUtils.cpp";

static const std::regex PREFIX_OF_RESERVED_TOPIC(R"(^\$aws\/rules\/[^\s]+?\/)", std::regex::icase);

bool MqttUtils::ValidateAwsIotMqttTopicName(std::string topic)
{
    std::smatch reserved_topic;
    if (std::regex_search(topic, reserved_topic, PREFIX_OF_RESERVED_TOPIC))
    {
        // Strip reserved topic prefix.
        topic = reserved_topic.suffix();
    }

    // Since std::string is based on char, the size of the string and number of UTF8 chars is same.
    if (topic.size() > MAX_LENGTH_OF_TOPIC)
    {
        LOGM_ERROR(TAG, "Number of bytes in topic (%zu) exceeds maximum (%zu)", topic.size(), MAX_LENGTH_OF_TOPIC);
        return false;
    }

    return true;
}
