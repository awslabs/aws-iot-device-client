// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../IntegrationTestResourceHandler.h"
#include <gtest/gtest.h>
#include <thread>

using namespace Aws;
using namespace Aws::Auth;
using namespace Aws::Client;
using namespace Aws::IoT;
using namespace Aws::IoT::Model;
using namespace std;

extern std::string THING_NAME;
extern std::string REGION;
extern std::shared_ptr<IntegrationTestResourceHandler> resourceHandler;

static const int WAIT_TIME = 1300;
static const int INTERVAL = 30;

class TestDeviceDefenderFeature : public testing::Test
{
  public:
    void SetUp() override
    {
        securityProfileName = "Integration-Test-Security-Profile-" + resourceHandler->GetTimeStamp();
        thingGroupName = "group-" + THING_NAME;

        resourceHandler->CreateThingGroup(thingGroupName);
        resourceHandler->AddThingToThingGroup(thingGroupName, THING_NAME);

        resourceHandler->CreateAndAttachSecurityProfile(securityProfileName, thingGroupName, metrics);
    }
    void TearDown() override { resourceHandler->DeleteSecurityProfile(securityProfileName); }
    string securityProfileName;
    string thingGroupName;
    vector<std::string> metrics{"aws:all-bytes-in", "aws:all-bytes-out", "aws:all-packets-in", "aws:all-packets-out"};
};

/**
 * To test Device Defender we are creating a Security Profile to create violations for any ( > 1) metrics output and
 * using this to facilitate testing. This is a simple test to verify the metrics are being emitted by the Device Client
 * by verifying metrics are causing violations. Verifying Packets In/Out & Bytes In/Out greater than 1.
 */

TEST_F(TestDeviceDefenderFeature, VerifyViolations)
{
    vector<ActiveViolation> violations;
    // Check for active violations for 10 minutes 30 seconds. Metrics interval is five minutes.
    int waitTime = WAIT_TIME;
    while (waitTime > 0)
    {
        violations = resourceHandler->GetViolations(securityProfileName);
        if (violations.size() == metrics.size())
        {
            break;
        }
        this_thread::sleep_for(std::chrono::seconds(INTERVAL));
        waitTime -= INTERVAL;
    }

    ASSERT_EQ(violations.size(), metrics.size());

    for (const ActiveViolation &violation : violations)
    {
        ASSERT_EQ(1, count(metrics.begin(), metrics.end(), violation.GetBehavior().GetMetric()));
    }
}
