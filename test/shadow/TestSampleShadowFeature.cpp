
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/Feature.h"
#include "../../source/shadow/SampleShadowFeature.h"
#include "gtest/gtest.h"

using namespace std;
using namespace Aws::Iot::DeviceClient::Shadow;

TEST(SampleShadowFeature, getName)
{
    SampleShadowFeature sampleShadowFeature;
    ASSERT_STREQ(sampleShadowFeature.getName().c_str(), "SampleShadow");
}