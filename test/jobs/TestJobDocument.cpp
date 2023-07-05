// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/SharedCrtResourceManager.h"
#include "../../source/jobs/JobDocument.h"
#include "../../source/util/UniqueString.h"

#include "gtest/gtest.h"
#include <algorithm>
#include <aws/crt/JsonObject.h>

using namespace std;
using namespace Aws::Crt;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::Jobs;

void AssertVectorEqual(const Optional<vector<string>> &vector1, const Optional<vector<string>> &vector2)
{
    ASSERT_TRUE(vector1->size() == vector2->size());
    ASSERT_TRUE(std::equal(vector1->begin(), vector1->end(), vector2->begin()));
}

void AssertConditionEqual(
    const vector<PlainJobDocument::JobCondition> condition1,
    const Optional<vector<PlainJobDocument::JobCondition>> condition2)
{
    ASSERT_TRUE(condition1.size() == condition2->size());
    for (auto iterator1 = condition1.cbegin(),
              end1 = condition1.cend(),
              iterator2 = condition2->cbegin(),
              end2 = condition2->cend();
         iterator1 != end1 || iterator2 != end2;
         ++iterator1, ++iterator2)
    {
        ASSERT_STREQ(iterator1->conditionKey.c_str(), iterator2->conditionKey.c_str());
        AssertVectorEqual(iterator1->conditionValue, iterator2->conditionValue);
        ASSERT_STREQ(iterator1->type->c_str(), iterator2->type->c_str());
    }
}

void AssertHandlerInputEqual(
    const Optional<PlainJobDocument::JobAction::ActionHandlerInput> &input1,
    const Optional<PlainJobDocument::JobAction::ActionHandlerInput> &input2)
{
    ASSERT_STREQ(input1->handler.c_str(), input2->handler.c_str());
    AssertVectorEqual(input1->args, input2->args);
    ASSERT_STREQ(input1->path->c_str(), input2->path->c_str());
}

void AssertCommandInputEqual(
    const Optional<PlainJobDocument::JobAction::ActionCommandInput> &input1,
    const Optional<PlainJobDocument::JobAction::ActionCommandInput> &input2)
{
    AssertVectorEqual(input1->command, input2->command);
}

void AssertStepEqual(const vector<PlainJobDocument::JobAction> &step1, const vector<PlainJobDocument::JobAction> &step2)
{
    ASSERT_TRUE(step1.size() == step2.size());
    for (auto iterator1 = step1.cbegin(), end1 = step1.cend(), iterator2 = step2.cbegin(), end2 = step2.cend();
         iterator1 != end1 || iterator2 != end2;
         ++iterator1, ++iterator2)
    {
        ASSERT_STREQ(iterator1->name.c_str(), iterator2->name.c_str());
        ASSERT_STREQ(iterator1->type.c_str(), iterator2->type.c_str());
        if (iterator1->type == "runHandler")
        {
            AssertHandlerInputEqual(iterator1->handlerInput, iterator2->handlerInput);
        }
        else
        {
            AssertCommandInputEqual(iterator1->commandInput, iterator2->commandInput);
        }
        ASSERT_STREQ(iterator1->runAsUser->c_str(), iterator2->runAsUser->c_str());
        ASSERT_TRUE(iterator1->allowStdErr.value() == iterator2->allowStdErr.value());
        ASSERT_TRUE(iterator2->ignoreStepFailure);
    }
}

TEST(JobDocument, SampleJobDocument)
{
    constexpr char jsonString[] = R"(
{
    "version": "1.0",
    "includeStdOut": "true",
    "conditions": [{
                    "key" : "operatingSystem",
                    "value": ["ubuntu", "redhat"],
                     "type": "stringEqual"
                 },
                 {
                    "key" : "OS",
                     "value": ["16.0"],
                     "type": "stringEqual"
    }],
    "steps": [{
            "action": {
                "name": "downloadJobHandler",
                "type": "runHandler",
                "input": {
                    "handler": "download-file.sh",
                    "args": ["presignedUrl", "/tmp/aws-iot-device-client/"],
                    "path": "path to handler"
                },
                "runAsUser": "user1",
                "allowStdErr": 8,
                "ignoreStepFailure": "true"
            }
        },
        {
            "action": {
                "name": "installApplicationAndReboot",
                "type": "runHandler",
                "input": {
                    "handler": "install-app.sh",
                    "args": [
                        "applicationName",
                        "active"
                    ],
                    "path": "path to handler"
                },
                "runAsUser": "user1",
                "allowStdErr": 8,
                "ignoreStepFailure": "true"
            }
        },
        {
            "action": {
                "name": "displayDirectory",
                "type": "runCommand",
                "input": {
                    "command": "ls,/tmp"
                },
                "runAsUser": "user1",
                "allowStdErr": 8,
                "ignoreStepFailure": "true"
            }
        },
        {
            "action": {
                "name": "validateAppStatus",
                "type": "runHandler",
                "input": {
                    "handler": "validate-app-status.sh",
                    "args": [
                        "applicationName",
                        "active"
                    ],
                    "path": "path to handler"
                },
                "runAsUser": "user1",
                "allowStdErr": 8,
                "ignoreStepFailure": "true"
            }
        }
    ],
    "finalStep": {
        "action": {
            "name": "deleteDownloadedHandler",
            "type": "runHandler",
            "input": {
                 "handler": "validate-app-status.sh",
                 "args": [
                    "applicationName",
                    "active"
                ],
                "path": "path to handler"
             },
            "runAsUser": "user1",
            "allowStdErr": 8,
            "ignoreStepFailure": "true"
        }
    }
})";

    // Initializing allocator, so we can use CJSON lib from SDK in our unit tests.
    SharedCrtResourceManager resourceManager;
    resourceManager.initializeAllocator();

    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainJobDocument jobDocument;
    jobDocument.LoadFromJobDocument(jsonView);

    for (const auto &i : jobDocument.steps)
    {
        cout << i.name.c_str() << "\n";
        if (i.type == "runHandler")
        {
            for (const auto &j : *i.handlerInput->args)
            {
                cout << j.c_str() << "\n";
            }
        }
        else
        {
            for (const auto &j : i.commandInput->command)
            {
                cout << j.c_str() << "\n";
            }
        }
    }

    ASSERT_TRUE(jobDocument.Validate());
    ASSERT_STREQ("1.0", jobDocument.version.c_str());
    ASSERT_TRUE(jobDocument.includeStdOut);

    vector<PlainJobDocument::JobCondition> conditions;
    PlainJobDocument::JobCondition condition1, condition2;
    condition1.conditionKey = "operatingSystem";
    condition1.conditionValue = {{"ubuntu"}, {"redhat"}};
    condition1.type = "stringEqual";
    conditions.push_back(condition1);

    condition2.conditionKey = "OS";
    condition2.conditionValue = {{"16.0"}};
    condition2.type = "stringEqual";
    conditions.push_back((condition2));

    AssertConditionEqual(conditions, jobDocument.conditions);

    for (const auto &i : conditions)
    {
        cout << i.type->c_str() << "\n";
    }
    for (const auto &i : *jobDocument.conditions)
    {
        cout << i.type->c_str() << "\n";
    }

    PlainJobDocument::JobAction action1, action2, action3, action4;
    PlainJobDocument::JobAction::ActionHandlerInput handlerInput1, handlerInput2, handlerInput4;
    PlainJobDocument::JobAction::ActionCommandInput commandInput3;
    std::vector<PlainJobDocument::JobAction> steps;

    action1.name = "downloadJobHandler";
    action1.type = "runHandler";
    handlerInput1.handler = "download-file.sh";
    handlerInput1.args = {{"presignedUrl"}, {"/tmp/aws-iot-device-client/"}};
    handlerInput1.path = "path to handler";
    action1.handlerInput = handlerInput1;
    action1.runAsUser = "user1";
    action1.allowStdErr = 8;
    action1.ignoreStepFailure = true;
    steps.push_back(action1);

    action2.name = "installApplicationAndReboot";
    action2.type = "runHandler";
    handlerInput2.handler = "install-app.sh";
    handlerInput2.args = {{"applicationName"}, {"active"}};
    handlerInput2.path = "path to handler";
    action2.handlerInput = handlerInput2;
    action2.runAsUser = "user1";
    action2.allowStdErr = 8;
    action2.ignoreStepFailure = true;
    steps.push_back(action2);

    action3.name = "displayDirectory";
    action3.type = "runCommand";
    commandInput3.command = {{"ls"}, {"/tmp"}};
    action3.commandInput = commandInput3;
    action3.runAsUser = "user1";
    action3.allowStdErr = 8;
    action3.ignoreStepFailure = true;
    steps.push_back(action3);

    action4.name = "validateAppStatus";
    action4.type = "runHandler";
    handlerInput4.handler = "validate-app-status.sh";
    handlerInput4.args = {{"applicationName"}, {"active"}};
    handlerInput4.path = "path to handler";
    action4.handlerInput = handlerInput4;
    action4.runAsUser = "user1";
    action4.allowStdErr = 8;
    action4.ignoreStepFailure = true;
    steps.push_back(action4);

    AssertStepEqual(steps, jobDocument.steps);

    PlainJobDocument::JobAction finalAction;
    PlainJobDocument::JobAction::ActionHandlerInput finalhandlerInput;
    finalAction.name = "deleteDownloadedHandler";
    finalAction.type = "runHandler";
    finalhandlerInput.handler = "validate-app-status.sh";
    finalhandlerInput.args = {{"applicationName"}, {"active"}};
    finalhandlerInput.path = "path to handler";
    finalAction.handlerInput = finalhandlerInput;
    finalAction.runAsUser = "user1";
    finalAction.allowStdErr = 8;
    finalAction.ignoreStepFailure = true;

    ASSERT_STREQ(finalAction.name.c_str(), jobDocument.finalStep->name.c_str());
    ASSERT_STREQ(finalAction.type.c_str(), jobDocument.finalStep->type.c_str());
    AssertHandlerInputEqual(finalAction.handlerInput, jobDocument.finalStep->handlerInput);
    ASSERT_STREQ(finalAction.runAsUser->c_str(), jobDocument.finalStep->runAsUser->c_str());
    ASSERT_EQ(finalAction.allowStdErr.value(), jobDocument.finalStep->allowStdErr.value());
    ASSERT_TRUE(jobDocument.finalStep->ignoreStepFailure);
}

TEST(JobDocument, MissingRequiredFields)
{
    constexpr char jsonString[] = R"(
{
    //version is missing
    "includeStdOut": "true",
    "conditions": [{
                    "key" : "operatingSystem",
                    "value": ["ubuntu", "redhat"],
                     "type": "stringEqual"
                 },
                 {
                    "key" : "OS",
                     "value": ["16.0"],
                     "type": "stringEqual"
    }],
    "steps": [{
            "action": {
                "name": "downloadJobHandler",
                "type": "runHandler",
                "input": {
                    "handler": "download-file.sh",
                    "args": ["presignedUrl", "/tmp/aws-iot-device-client/"],
                    "path": "path to handler"
                },
                "runAsUser": "user1",
                "allowStdErr": "8",
                "ignoreStepFailure": "true"
            }
        },
        {
            "action": {
                "name": "installApplicationAndReboot",
                "type": "runHandler",
                "input": {
                    "handler": "install-app.sh",
                    "args": [
                        "applicationName",
                        "active"
                    ],
                    "path": "path to handler"
                },
                "runAsUser": "user1",
                "allowStdErr": "8",
                "ignoreStepFailure": "true"
            }
        },
        {
            "action": {
                "name": "validateAppStatus",
                "type": "runHandler",
                "input": {
                    "handler": "validate-app-status.sh",
                    "args": [
                        "applicationName",
                        "active"
                    ],
                    "path": "path to handler"
                },
                "runAsUser": "user1",
                "allowStdErr": "8",
                "ignoreStepFailure": "true"
            }
        }
    ],
    "finalStep": {
        "action": {
            "name": "deleteDownloadedHandler",
            "type": "runHandler",
            "input": {
                 "handler": "validate-app-status.sh",
                 "args": [
                    "applicationName",
                    "active"
                ],
                "path": "path to handler"
             },
            "runAsUser": "user1",
            "allowStdErr": "8",
            "ignoreStepFailure": "true"
        }
    }
    })";

    // Initializing allocator, so we can use CJSON lib from SDK in our unit tests.
    SharedCrtResourceManager resourceManager;
    resourceManager.initializeAllocator();

    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainJobDocument jobDocument;
    jobDocument.LoadFromJobDocument(jsonView);

    ASSERT_FALSE(jobDocument.Validate());
}

TEST(JobDocument, MinimumJobDocument)
{
    constexpr char jsonString[] = R"(
{
    "version": "1.0",
    "steps": [{
            "action": {
                "name": "downloadJobHandler",
                "type": "runHandler",
                "input": {
                    "handler": "download-file.sh"
                }
            }
        },
        {
            "action": {
                "name": "installApplicationAndReboot",
                "type": "runHandler",
                "input": {
                    "handler": "install-app.sh"
                }
            }
        },
        {
            "action": {
                "name": "displayDirectory",
                "type": "runCommand",
                "input": {
                    "command": "ls,/tmp"
                }
            }
        },
        {
            "action": {
                "name": "validateAppStatus",
                "type": "runHandler",
                "input": {
                    "handler": "validate-app-status.sh"
                }
            }
        }
    ]
})";

    // Initializing allocator, so we can use CJSON lib from SDK in our unit tests.
    SharedCrtResourceManager resourceManager;
    resourceManager.initializeAllocator();

    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainJobDocument jobDocument;
    jobDocument.LoadFromJobDocument(jsonView);

    ASSERT_TRUE(jobDocument.Validate());
}

TEST(JobDocument, MissingRequiredFieldsValue)
{
    constexpr char jsonString[] = R"(
{
    "version": "1.0",
    "includeStdOut": "true",
    "conditions": [{
                    "key" : "operatingSystem",
                    "value": ["ubuntu", "redhat"],
                     "type": "stringEqual"
                 },
                 {
                    "key" : "OS",
                     "value": ["16.0"],
                     "type": "stringEqual"
    }],
    "steps": [{
            "action": {
                "name": ,
                "type": "runHandler",
                "input": {
                    "handler": "download-file.sh",
                    "args": ["presignedUrl", "/tmp/aws-iot-device-client/"],
                    "path": "path to handler"
                },
                "runAsUser": "user1",
                "allowStdErr": "8",
                "ignoreStepFailure": "true"
            }
        },
        {
            "action": {
                "name": "installApplicationAndReboot",
                "type": "runHandler",
                "input": {
                    "handler": "install-app.sh",
                    "args": [
                        "applicationName",
                        "active"
                    ],
                    "path": "path to handler"
                },
                "runAsUser": "user1",
                "allowStdErr": "8",
                "ignoreStepFailure": "true"
            }
        },
        {
            "action": {
                "name": "validateAppStatus",
                "type": "runHandler",
                "input": {
                    "handler": "validate-app-status.sh",
                    "args": [
                        "applicationName",
                        "active"
                    ],
                    "path": "path to handler"
                },
                "runAsUser": "user1",
                "allowStdErr": "8",
                "ignoreStepFailure": "true"
            }
        }
    ],
    "finalStep": {
        "action": {
            "name": "deleteDownloadedHandler",
            "type": "runHandler",
            "input": {
                 "handler": "validate-app-status.sh",
                 "args": [
                    "applicationName",
                    "active"
                ],
                "path": "path to handler"
             },
            "runAsUser": "user1",
            "allowStdErr": "8",
            "ignoreStepFailure": "true"
        }
    }
    })";

    // Initializing allocator, so we can use CJSON lib from SDK in our unit tests.
    SharedCrtResourceManager resourceManager;
    resourceManager.initializeAllocator();

    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainJobDocument jobDocument;
    jobDocument.LoadFromJobDocument(jsonView);

    ASSERT_FALSE(jobDocument.Validate());
}

TEST(JobDocument, CommandFieldsIsEmpty)
{
    constexpr char jsonString[] = R"(
{
    "version": "1.0",
    "steps": [
        {
            "action": {
                "name": "displayDirectory",
                "type": "runCommand",
                "input": {
                    "command":
                }
            }
        }
    ]
})";

    // Initializing allocator, so we can use CJSON lib from SDK in our unit tests.
    SharedCrtResourceManager resourceManager;
    resourceManager.initializeAllocator();

    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainJobDocument jobDocument;
    jobDocument.LoadFromJobDocument(jsonView);

    ASSERT_FALSE(jobDocument.Validate());
}

TEST(JobDocument, CommandContainsSpaceCharacters)
{
    constexpr char jsonString[] = R"(
{
    "version": "1.0",
    "steps": [
        {
            "action": {
                "name": "displayDirectory",
                "type": "runCommand",
                "input": {
                    "command": " \n echo \t, Hello World "
                }
            }
        }
    ]
})";

    // Initializing allocator, so we can use CJSON lib from SDK in our unit tests.
    SharedCrtResourceManager resourceManager;
    resourceManager.initializeAllocator();

    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainJobDocument jobDocument;
    jobDocument.LoadFromJobDocument(jsonView);

    ASSERT_TRUE(jobDocument.Validate());
}

TEST(JobDocument, SpaceCharactersContainedWithinFirstWordOfCommand)
{
    constexpr char jsonString[] = R"(
{
    "version": "1.0",
    "steps": [
        {
            "action": {
                "name": "displayDirectory",
                "type": "runCommand",
                "input": {
                    "command": " aws iot \t,describe-endpoint"
                }
            }
        }
    ]
})";

    // Initializing allocator, so we can use CJSON lib from SDK in our unit tests.
    SharedCrtResourceManager resourceManager;
    resourceManager.initializeAllocator();

    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainJobDocument jobDocument;
    jobDocument.LoadFromJobDocument(jsonView);

    ASSERT_FALSE(jobDocument.Validate());
}

TEST(JobDocument, oldJobDocumentCompatibility)
{
    constexpr char jsonString[] = R"(
{
    "operation": "download-file.sh",
    "args": ["https://github.com/awslabs/aws-iot-device-client/archive/refs/tags/v1.3.tar.gz", "/tmp/Downloaded_File.tar.gz"],
    "path": "default"
})";

    // Initializing allocator, so we can use CJSON lib from SDK in our unit tests.
    SharedCrtResourceManager resourceManager;
    resourceManager.initializeAllocator();

    JsonObject jsonObject(jsonString);
    JsonView jsonView = jsonObject.View();

    PlainJobDocument jobDocument;
    jobDocument.LoadFromJobDocument(jsonView);

    ASSERT_TRUE(jobDocument.Validate());
}