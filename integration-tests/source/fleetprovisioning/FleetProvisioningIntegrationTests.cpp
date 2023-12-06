// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../IntegrationTestResourceHandler.h"
#include <aws/iot/model/CreateJobRequest.h>
#include <gtest/gtest.h>
#include <thread>

using namespace Aws;
using namespace Aws::Auth;
using namespace Aws::Client;
using namespace Aws::IoT;
using namespace Aws::IoT::Model;
using namespace std;

static constexpr char FP_JOB_ID[] = "Fleet-Provisioning-Test-HappyPath-";
static constexpr char HEALTH_CHECK_JOB_DOC[] = "{\"operation\": \"health-check.sh\", \"path\": \"default\"}";

extern std::string THING_NAME;
extern std::string REGION;
extern std::string PORT;
extern bool SKIP_FP;
extern std::shared_ptr<IntegrationTestResourceHandler> resourceHandler;

class TestFleetProvisioningFeature : public ::testing::Test
{
  public:
    void SetUp() override
    {
        if (SKIP_FP)
        {
            GTEST_SKIP();
        }
    }
    void TearDown() override { resourceHandler->CleanUp(); }
};

TEST_F(TestFleetProvisioningFeature, HappyPath)
{
    string jobId = FP_JOB_ID + resourceHandler->GetTimeStamp();
    resourceHandler->CreateJob(jobId, HEALTH_CHECK_JOB_DOC);

    ASSERT_EQ(resourceHandler->GetJobExecutionStatusWithRetry(jobId), JobExecutionStatus::SUCCEEDED);
}
