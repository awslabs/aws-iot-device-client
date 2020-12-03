// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/Feature.h"
#include "../../source/jobs/JobsFeature.h"
#include "gtest/gtest.h"

using namespace std;
using namespace Aws::Iot::DeviceClient::Jobs;

TEST(JobsFeature, getName)
{
    JobsFeature jobs;
    ASSERT_STREQ(jobs.getName().c_str(), "Jobs");
}
