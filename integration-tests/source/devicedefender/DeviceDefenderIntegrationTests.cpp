// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../IntegrationTestResourceHandler.h"
#include <aws/core/Aws.h>
#include <aws/iot/IoTClient.h>
#include <gtest/gtest.h>

using namespace Aws;
using namespace Aws::Auth;
using namespace Aws::Client;
using namespace Aws::IoT;
using namespace Aws::IoT::Model;
using namespace std;

extern std::string THING_NAME;
extern std::string REGION;

class TestDeviceDefenderFeature : public ::testing::Test
{
  public:
    // cppcheck-suppress unusedFunction
    void SetUp() override
    {
        SDKOptions options;
        Aws::InitAPI(options);
        {
            ClientConfiguration clientConfig;
            clientConfig.region = REGION;
            resourceHandler =
                unique_ptr<IntegrationTestResourceHandler>(new IntegrationTestResourceHandler(clientConfig));

            resourceHandler->CreateAndAttachSecurityProfile(THING_NAME, metrics);
        }
    }
    void TearDown() override
    {
        resourceHandler->DeleteSecurityProfile(THING_NAME);
        SDKOptions options;
        Aws::ShutdownAPI(options);
    }
    unique_ptr<IntegrationTestResourceHandler> resourceHandler;
    vector<std::string> metrics{"aws:all-bytes-in", "aws:all-bytes-out", "aws:all-packets-in", "aws:all-packets-out"};
};

/**
 * To test Device Defender we are creating a Security Profile to create violations for any ( > 1) metrics output and using this
 * to facilitate testing. This is a simple test to verify the metrics are being emitted by the Device Client by verifying
 * metrics are causing violations. Verifying Packets In/Out & Bytes In/Out greater than 1.
 */

TEST_F(TestDeviceDefenderFeature, VerifyViolations)
{
    vector<ActiveViolation> violations;
    int maxWaitTime = 630;
    while (maxWaitTime > 0)
    {
        violations = resourceHandler->GetViolations(THING_NAME);
        if (violations.size() == metrics.size())
        {
            break;
        }
        this_thread::sleep_for(std::chrono::seconds(10));
        maxWaitTime -= 10;
    }

    ASSERT_EQ(violations.size(), metrics.size());

    for (const ActiveViolation &violation : violations)
    {
        ASSERT_EQ(1, count(metrics.begin(), metrics.end(), violation.GetBehavior().GetMetric()));
    }
}
