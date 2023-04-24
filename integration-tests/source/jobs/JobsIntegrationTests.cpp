// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../IntegrationTestResourceHandler.h"
#include <aws/iot/model/CreateJobRequest.h>
#include <gtest/gtest.h>

using namespace Aws;
using namespace Aws::Auth;
using namespace Aws::Client;
using namespace Aws::IoT;
using namespace Aws::IoT::Model;
using namespace std;

extern std::string THING_NAME;
extern std::string REGION;
extern std::shared_ptr<IntegrationTestResourceHandler> resourceHandler;

static constexpr char DOWNLOAD_FILE_JOB_DOC[] =
    "{ \"version\": \"1.0\", \"steps\": [{ \"action\": { \"name\": \"Download File\", \"type\": \"runHandler\", "
    "\"input\": { \"handler\": \"download-file.sh\", \"args\": [ "
    "\"https://github.com/awslabs/aws-iot-device-client/blob/main/README.md\", "
    "\"/tmp/README.md\" ],\"path\": \"default\" },\"runAsUser\": \"root\" } } ] }";
static constexpr char INSTALL_PACKAGES_JOB_DOC[] =
    "{ \"version\": \"1.0\", \"steps\": [ { \"action\": { \"name\": \"Install Packages\", \"type\": \"runHandler\", "
    "\"input\": { \"handler\": \"install-packages.sh\", \"args\": [ \"dos2unix\" ], \"path\": \"default\" }, "
    "\"runAsUser\": \"root\" } }]}";
static constexpr char VERIFY_PACKAGES_INSTALLED_JOB_DOC[] =
    "{ \"version\": \"1.0\", \"steps\": [ { \"action\": { \"name\": \"Verify Packages Installed\", \"type\": "
    "\"runHandler\", \"input\": { \"handler\": \"verify-packages-installed.sh\", \"args\": [ \"dos2unix\" ], \"path\": "
    "\"default\" }, \"runAsUser\": \"root\" } }]}";
static constexpr char REMOVE_PACKAGES_JOB_DOC[] =
    "{ \"version\": \"1.0\", \"steps\": [ { \"action\": { \"name\": \"Remove Packages\", \"type\": \"runHandler\", "
    "\"input\": { \"handler\": \"remove-packages.sh\", \"args\": [ \"dos2unix\" ], \"path\": \"default\" }, "
    "\"runAsUser\": \"root\" } }]}";
static constexpr char VERIFY_PACKAGES_REMOVED_JOB_DOC[] =
    "{ \"version\": \"1.0\", \"steps\": [ { \"action\": { \"name\": \"Verify Packages Removed\", \"type\": "
    "\"runHandler\", \"input\": { \"handler\": \"verify-packages-removed.sh\", \"args\": [ \"dos2unix\" ], \"path\": "
    "\"default\" }, \"runAsUser\": \"root\" } }]}";
static constexpr char RUN_COMMAND_PRINT_GREETING_JOB_DOC[] =
    "{ \"version\": \"1.0\", \"steps\": [ { \"action\": { \"name\": \"Print Greeting\", \"type\": "
    "\"runCommand\", \"input\": { \"command\": \"echo,Hello World\" }, \"runAsUser\": \"root\" } }]}";

class TestJobsFeature : public ::testing::Test
{
  public:
    void SetUp() override {}
    void TearDown() override { resourceHandler->CleanUp(); }
};

TEST_F(TestJobsFeature, InstallPackages)
{
    string jobId = "Install-Packages-" + resourceHandler->GetTimeStamp();
    resourceHandler->CreateJob(jobId, INSTALL_PACKAGES_JOB_DOC);

    ASSERT_EQ(resourceHandler->GetJobExecutionStatusWithRetry(jobId), JobExecutionStatus::SUCCEEDED);

    jobId = "Verify-Packages-Installed-" + resourceHandler->GetTimeStamp();
    resourceHandler->CreateJob(jobId, VERIFY_PACKAGES_INSTALLED_JOB_DOC);

    ASSERT_EQ(resourceHandler->GetJobExecutionStatusWithRetry(jobId), JobExecutionStatus::SUCCEEDED);
}

TEST_F(TestJobsFeature, RemovePackages)
{
    string jobId = "Remove-Packages-" + resourceHandler->GetTimeStamp();
    resourceHandler->CreateJob(jobId, REMOVE_PACKAGES_JOB_DOC);

    ASSERT_EQ(resourceHandler->GetJobExecutionStatusWithRetry(jobId), JobExecutionStatus::SUCCEEDED);

    jobId = "Verify-Packages-Removed-" + resourceHandler->GetTimeStamp();
    resourceHandler->CreateJob(jobId, VERIFY_PACKAGES_REMOVED_JOB_DOC);

    ASSERT_EQ(resourceHandler->GetJobExecutionStatusWithRetry(jobId), JobExecutionStatus::SUCCEEDED);
}

TEST_F(TestJobsFeature, DownloadFile)
{
    string jobId = "Download-File-" + resourceHandler->GetTimeStamp();
    resourceHandler->CreateJob(jobId, DOWNLOAD_FILE_JOB_DOC);

    ASSERT_EQ(resourceHandler->GetJobExecutionStatusWithRetry(jobId), JobExecutionStatus::SUCCEEDED);
}

TEST_F(TestJobsFeature, PrintGreeting)
{
    string jobId = "Print-Greeting-" + resourceHandler->GetTimeStamp();
    resourceHandler->CreateJob(jobId, RUN_COMMAND_PRINT_GREETING_JOB_DOC);

    ASSERT_EQ(resourceHandler->GetJobExecutionStatusWithRetry(jobId), JobExecutionStatus::SUCCEEDED);
}