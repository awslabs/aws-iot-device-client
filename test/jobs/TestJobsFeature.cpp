// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "../../source/Feature.h"
#include "../../source/jobs/JobsFeature.h"

using namespace std;
using namespace Aws::Iot::DeviceClient;

TEST(JobsFeature, getName)
{
    JobsFeature jobs;
    ASSERT_STREQ(jobs.get_name().c_str(),"Jobs");
}
