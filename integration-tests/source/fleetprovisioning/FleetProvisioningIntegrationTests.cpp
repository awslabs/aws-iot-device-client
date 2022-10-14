// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../IntegrationTestResourceHandler.h"
#include <aws/core/Aws.h>
#include <aws/iot/IoTClient.h>
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

class TestFleetProvisioningFixture : public ::testing::Test
{
  public:
    // cppcheck-suppress unusedFunction
    void SetUp() override
    {
        if (!SKIP_FP)
        {
            Aws::InitAPI(options);
            {
                ClientConfiguration clientConfig;
                clientConfig.region = REGION;
                resourceHandler =
                    unique_ptr<IntegrationTestResourceHandler>(new IntegrationTestResourceHandler(clientConfig));
            }
        }
        else
        {
            printf("Skipping Fleet Provisioning Tests. \n");
            GTEST_SKIP();
        }
    }
    void TearDown() override
    {
        if (!SKIP_FP)
        {
            resourceHandler->CleanUp();
            Aws::ShutdownAPI(options);
        }
    }
    SDKOptions options;
    unique_ptr<IntegrationTestResourceHandler> resourceHandler;
};

TEST_F(TestFleetProvisioningFixture, HappyPath)
{
    string jobId = FP_JOB_ID + resourceHandler->GetTimeStamp();
    resourceHandler->CreateJob(jobId, HEALTH_CHECK_JOB_DOC);

    ASSERT_EQ(resourceHandler->GetJobExecutionStatusWithRetry(jobId), JobExecutionStatus::SUCCEEDED);
}
