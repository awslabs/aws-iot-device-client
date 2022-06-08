// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/jobs/JobEngine.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <fstream>

using namespace std;
using namespace Aws;
using namespace Aws::Iot;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::Util;
using namespace Aws::Iot::DeviceClient::Jobs;

PlainJobDocument::JobAction createJobAction(
    string name,
    string type,
    string handler,
    std::vector<std::string> args,
    string path,
    bool ignoreStepFailure)
{
    PlainJobDocument::JobAction::ActionInput input;
    input.handler = handler;
    input.args = args;
    input.path = path;

    PlainJobDocument::JobAction action;
    action.name = name;
    action.type = type;
    action.ignoreStepFailure = ignoreStepFailure;
    action.input = input;

    return action;
}

PlainJobDocument createTestJobDocument(vector<PlainJobDocument::JobAction> steps, bool includeStdOut)
{
    PlainJobDocument jobDocument;
    jobDocument.version = "1.0";
    jobDocument.includeStdOut = includeStdOut;
    jobDocument.steps = steps;

    return jobDocument;
}

PlainJobDocument createTestJobDocument(
    vector<PlainJobDocument::JobAction> steps,
    PlainJobDocument::JobAction finalStep,
    bool includeStdOut)
{
    PlainJobDocument jobDocument = createTestJobDocument(steps, includeStdOut);
    jobDocument.finalStep = finalStep;
    return jobDocument;
}

const string testHandlerDirectoryPath = "/tmp/device-client-tests";
const string successHandlerPath = testHandlerDirectoryPath + "/successHandler";
const string errorHandlerPath = testHandlerDirectoryPath + "/errorHandler";

const string testStdout = "This is test stdout";
const string testStderr = "This is test stderr";
const string successHandlerScript = "echo \"" + testStdout + "\"";
const string errorHandlerScript = "1>&2 echo \"" + testStderr + "\"; exit 1";

class TestJobEngine : public testing::Test
{
  public:
    void SetUp() override
    {
        Util::FileUtils::CreateDirectoryWithPermissions(testHandlerDirectoryPath.c_str(), 0700);

        ofstream successHandler(successHandlerPath, std::fstream::app);
        chmod(successHandlerPath.c_str(), 0700);
        successHandler << successHandlerScript << endl;

        ofstream errorHandler(errorHandlerPath, std::fstream::app);
        chmod(errorHandlerPath.c_str(), 0700);
        errorHandler << errorHandlerScript << endl;
    }

    void TearDown() override
    {
        std::remove(successHandlerPath.c_str());
        std::remove(errorHandlerPath.c_str());
        std::remove(testHandlerDirectoryPath.c_str());
    }
};

TEST_F(TestJobEngine, ExecuteStepsHappy)
{
    vector<PlainJobDocument::JobAction> steps;
    vector<std::string> args;
    steps.push_back(
        createJobAction("testAction", "runHandler", "successHandler", args, "/tmp/device-client-tests/", false));
    PlainJobDocument jobDocument = createTestJobDocument(steps, true);
    JobEngine jobEngine;

    int executionStatus = jobEngine.exec_steps(jobDocument, testHandlerDirectoryPath);
    ASSERT_EQ(executionStatus, 0);
    ASSERT_STREQ(jobEngine.getStdOut().c_str(), std::string(testStdout + "\n").c_str());
}

TEST_F(TestJobEngine, ExecuteSucceedThenFail)
{
    vector<PlainJobDocument::JobAction> steps;
    vector<std::string> args;
    steps.push_back(
        createJobAction("testAction", "runHandler", "successHandler", args, "/tmp/device-client-tests/", false));
    PlainJobDocument::JobAction finalStep =
        createJobAction("testAction", "runHandler", "errorHandler", args, "/tmp/device-client-tests/", false);
    PlainJobDocument jobDocument = createTestJobDocument(steps, finalStep, true);
    JobEngine jobEngine;

    int executionStatus = jobEngine.exec_steps(jobDocument, testHandlerDirectoryPath);
    ASSERT_NE(executionStatus, 0);
    ASSERT_STREQ(jobEngine.getStdOut().c_str(), std::string(testStdout + "\n").c_str());
    ASSERT_STREQ(jobEngine.getStdErr().c_str(), std::string(testStderr + "\n").c_str());
}

TEST_F(TestJobEngine, ExecuteFinalStepOnly)
{
    vector<PlainJobDocument::JobAction> steps;
    vector<std::string> args;
    PlainJobDocument::JobAction finalStep =
        createJobAction("testAction", "runHandler", "successHandler", args, "/tmp/device-client-tests/", false);
    PlainJobDocument jobDocument = createTestJobDocument(steps, finalStep, true);
    JobEngine jobEngine;

    int executionStatus = jobEngine.exec_steps(jobDocument, testHandlerDirectoryPath);
    ASSERT_EQ(executionStatus, 0);
    ASSERT_STREQ(jobEngine.getStdOut().c_str(), std::string(testStdout + "\n").c_str());
}

TEST_F(TestJobEngine, ExecuteStepsError)
{
    vector<PlainJobDocument::JobAction> steps;
    vector<std::string> args;
    steps.push_back(
        createJobAction("testAction", "runHandler", "errorHandler", args, "/tmp/device-client-tests/", false));
    PlainJobDocument jobDocument = createTestJobDocument(steps, true);
    JobEngine jobEngine;

    int executionStatus = jobEngine.exec_steps(jobDocument, testHandlerDirectoryPath);
    ASSERT_NE(executionStatus, 0);
    ASSERT_STREQ(jobEngine.getStdErr().c_str(), std::string(testStderr + "\n").c_str());
}

TEST_F(TestJobEngine, ExecuteNoSteps)
{
    vector<PlainJobDocument::JobAction> steps;
    vector<std::string> args;
    PlainJobDocument jobDocument = createTestJobDocument(steps, true);
    JobEngine jobEngine;

    int executionStatus = jobEngine.exec_steps(jobDocument, testHandlerDirectoryPath);
    ASSERT_EQ(executionStatus, 0);
    ASSERT_EQ(jobEngine.getStdOut().length(), 0);
    ASSERT_EQ(jobEngine.getStdErr().length(), 0);
}