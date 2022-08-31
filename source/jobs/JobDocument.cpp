// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "JobDocument.h"
#include "../logging/LoggerFactory.h"
#include <aws/crt/JsonObject.h>

using namespace std;
using namespace Aws::Iot::DeviceClient::Jobs;
using namespace Aws::Iot;
using namespace Aws::Crt;
using namespace Aws::Iot::DeviceClient::Logging;

constexpr char LoadableFromJobDocument::TAG[];

constexpr char PlainJobDocument::ACTION_TYPE_RUN_HANDLER[];
constexpr char PlainJobDocument::ACTION_TYPE_RUN_COMMAND[];

constexpr char PlainJobDocument::JSON_KEY_VERSION[];
constexpr char PlainJobDocument::JSON_KEY_INCLUDESTDOUT[];
constexpr char PlainJobDocument::JSON_KEY_CONDITIONS[];
constexpr char PlainJobDocument::JSON_KEY_STEPS[];
constexpr char PlainJobDocument::JSON_KEY_ACTION[];
constexpr char PlainJobDocument::JSON_KEY_FINALSTEP[];
// Old Schema fields
constexpr char PlainJobDocument::JSON_KEY_OPERATION[];
constexpr char PlainJobDocument::JSON_KEY_ARGS[];
constexpr char PlainJobDocument::JSON_KEY_ALLOWSTDERR[];
constexpr char PlainJobDocument::JSON_KEY_PATH[];
// Old Schema default values
constexpr char PlainJobDocument::OLD_SCHEMA_VERSION[];

void PlainJobDocument::LoadFromJobDocument(const JsonView &json)
{
    const char *jsonKey = JSON_KEY_VERSION;
    if (json.ValueExists(jsonKey) && json.GetJsonObject(jsonKey).IsString())
    {
        version = json.GetString(jsonKey).c_str();
    }

    jsonKey = JSON_KEY_INCLUDESTDOUT;
    if (json.ValueExists(jsonKey) && json.GetJsonObject(jsonKey).IsString())
    {
        includeStdOut = json.GetString(jsonKey) == "true";
    }

    if (version.empty())
    {
        //  Converting Old Job Document schema to new Job Document schema
        version = OLD_SCHEMA_VERSION;

        jsonKey = JSON_KEY_OPERATION;
        if (json.ValueExists(jsonKey) && json.GetJsonObject(jsonKey).IsString())
        {
            JobAction jobAction;
            // Save Job Action name and handler field value with operation field value
            jobAction.name = json.GetString(jsonKey).c_str();
            jobAction.input.handler = jobAction.name;

            jsonKey = JSON_KEY_ARGS;
            if (json.ValueExists(jsonKey) && json.GetJsonObject(jsonKey).IsListType())
            {
                jobAction.input.args = ParseToVectorString(json.GetJsonObject(jsonKey));
            }

            // Old Schema only supports runHandler type of action
            jobAction.type = ACTION_TYPE_RUN_HANDLER;

            jsonKey = JSON_KEY_ALLOWSTDERR;
            if (json.ValueExists(jsonKey) && json.GetJsonObject(jsonKey).IsIntegerType())
            {
                jobAction.allowStdErr = json.GetInteger(jsonKey);
            }

            jsonKey = JSON_KEY_PATH;
            if (json.ValueExists(jsonKey) && json.GetJsonObject(jsonKey).IsString())
            {
                jobAction.input.path = json.GetString(jsonKey).c_str();
            }

            steps.push_back(jobAction);
        }
    }
    else
    {
        // Job received with new Job Document Schema structure

        jsonKey = JSON_KEY_CONDITIONS;
        if (json.ValueExists(jsonKey) && json.GetJsonObject(jsonKey).IsListType())
        {
            conditions = std::vector<PlainJobDocument::JobCondition>();
            for (const auto &condition : json.GetArray(jsonKey))
            {
                JobCondition temp;
                temp.LoadFromJobDocument(condition);
                conditions->push_back(temp);
            }
        }

        jsonKey = JSON_KEY_STEPS;
        if (json.ValueExists(jsonKey) && json.GetJsonObject(jsonKey).IsListType())
        {
            for (const auto &action : json.GetArray(jsonKey))
            {
                const char *jsonKeyTwo = JSON_KEY_ACTION;

                if (action.ValueExists(jsonKeyTwo))
                {
                    JobAction temp;
                    temp.LoadFromJobDocument(action.GetJsonObject(jsonKeyTwo));
                    steps.push_back(temp);
                }
            }
        }

        jsonKey = JSON_KEY_FINALSTEP;
        if (json.ValueExists(jsonKey))
        {
            const char *jsonKeyTwo = JSON_KEY_ACTION;
            const auto &finalAction = json.GetJsonObject(jsonKey);
            if (finalAction.ValueExists(jsonKeyTwo))
            {
                JobAction temp;
                temp.LoadFromJobDocument(finalAction.GetJsonObject(jsonKeyTwo));
                finalStep = temp;
            }
        }
    }
}

vector<string> PlainJobDocument::ParseToVectorString(const JsonView &json)
{
    vector<string> plainVector;

    for (const auto &i : json.AsArray())
    {
        // cppcheck-suppress useStlAlgorithm
        plainVector.push_back(i.AsString().c_str());
    }
    return plainVector;
}

bool PlainJobDocument::Validate() const
{
    if (version.empty())
    {
        LOGM_ERROR(TAG, "*** %s: Required field Version is missing ***", DeviceClient::Jobs::DC_INVALID_JOB_DOC);
        return false;
    }

    if (conditions.has_value())
    {
        for (const auto &condition : *conditions)
        {
            if (!condition.Validate())
            // cppcheck-suppress useStlAlgorithm
            {
                return false;
            }
        }
    }

    if (steps.empty())
    {
        LOGM_ERROR(TAG, "*** %s: Required field Steps is missing ***", DeviceClient::Jobs::DC_INVALID_JOB_DOC);
        return false;
    }
    else
    {
        for (const auto &action : steps)
        {
            if (!action.Validate())
            // cppcheck-suppress useStlAlgorithm
            {
                return false;
            }
        }
    }

    if (finalStep.has_value())
    {
        if (!finalStep->Validate())
        {
            return false;
        }
    }
    return true;
}

constexpr char PlainJobDocument::JobCondition::JSON_KEY_CONDITION_KEY[];
constexpr char PlainJobDocument::JobCondition::JSON_KEY_CONDITION_VALUE[];
constexpr char PlainJobDocument::JobCondition::JSON_KEY_TYPE[];

void PlainJobDocument::JobCondition::LoadFromJobDocument(const JsonView &json)
{
    const char *jsonKey = JSON_KEY_CONDITION_KEY;
    if (json.ValueExists(jsonKey) && json.GetJsonObject(jsonKey).IsString())
    {
        conditionKey = json.GetString(jsonKey).c_str();
    }

    jsonKey = JSON_KEY_CONDITION_VALUE;
    if (json.ValueExists(jsonKey) && json.GetJsonObject(jsonKey).IsListType())
    {
        conditionValue = ParseToVectorString(json.GetJsonObject(jsonKey));
    }

    jsonKey = JSON_KEY_TYPE;
    if (json.ValueExists(jsonKey) && json.GetJsonObject(jsonKey).IsString())
    {
        type = json.GetString(jsonKey).c_str();
    }
}

bool PlainJobDocument::JobCondition::Validate() const
{
    if (conditionKey.empty())
    {
        LOGM_ERROR(TAG, "*** %s: Required field Condition Key is missing ***", DeviceClient::Jobs::DC_INVALID_JOB_DOC);
        return false;
    }
    if (conditionValue.empty())
    {
        LOGM_ERROR(
            TAG, "*** %s: Required field Condition Value is missing ***", DeviceClient::Jobs::DC_INVALID_JOB_DOC);
        return false;
    }
    return true;
}

constexpr char PlainJobDocument::JobAction::JSON_KEY_NAME[];
constexpr char PlainJobDocument::JobAction::JSON_KEY_TYPE[];
constexpr char PlainJobDocument::JobAction::JSON_KEY_INPUT[];
constexpr char PlainJobDocument::JobAction::JSON_KEY_RUNASUSER[];
constexpr char PlainJobDocument::JobAction::JSON_KEY_ALLOWSTDERR[];
constexpr char PlainJobDocument::JobAction::JSON_KEY_IGNORESTEPFAILURE[];
std::string currentType;

void PlainJobDocument::JobAction::LoadFromJobDocument(const JsonView &json)
{
    const char *jsonKey = JSON_KEY_NAME;
    if (json.ValueExists(jsonKey) && json.GetJsonObject(jsonKey).IsString())
    {
        name = json.GetString(jsonKey).c_str();
    }

    jsonKey = JSON_KEY_TYPE;
    if (json.ValueExists(jsonKey) && json.GetJsonObject(jsonKey).IsString())
    {
        type = json.GetString(jsonKey).c_str();
        currentType = type;
    }

    jsonKey = JSON_KEY_INPUT;
    if (json.ValueExists(jsonKey))
    {
        ActionInput temp;
        temp.LoadFromJobDocument(json.GetJsonObject(jsonKey));
        input = temp;
    }

    jsonKey = JSON_KEY_RUNASUSER;
    if (json.ValueExists(jsonKey) && json.GetJsonObject(jsonKey).IsString())
    {
        runAsUser = json.GetString(jsonKey).c_str();
    }

    jsonKey = JSON_KEY_ALLOWSTDERR;
    if (json.ValueExists(jsonKey) && json.GetJsonObject(jsonKey).IsIntegerType())
    {
        allowStdErr = json.GetInteger(jsonKey);
    }

    jsonKey = JSON_KEY_IGNORESTEPFAILURE;
    if (json.ValueExists(jsonKey) && json.GetJsonObject(jsonKey).IsString())
    {
        ignoreStepFailure = json.GetString(jsonKey) == "true";
    }
}

bool PlainJobDocument::JobAction::Validate() const
{
    if (name.empty())
    {
        LOGM_ERROR(TAG, "*** %s: Required field Action Name is missing ***", DeviceClient::Jobs::DC_INVALID_JOB_DOC);
        return false;
    }

    if (type.empty())
    {
        LOGM_ERROR(TAG, "*** %s: Required field Action Type is missing ***", DeviceClient::Jobs::DC_INVALID_JOB_DOC);
        return false;
    }

    if (type != ACTION_TYPE_RUN_HANDLER && type != ACTION_TYPE_RUN_COMMAND)
    {
        LOGM_ERROR(
            TAG,
            "*** %s: Required field Action Type with invalid value: %s ***",
            DeviceClient::Jobs::DC_INVALID_JOB_DOC,
            type.c_str());
        return false;
    }

    if (!input.Validate())
    {
        return false;
    }

    return true;
}

constexpr char PlainJobDocument::JobAction::ActionInput::JSON_KEY_HANDLER[];
constexpr char PlainJobDocument::JobAction::ActionInput::JSON_KEY_ARGS[];
constexpr char PlainJobDocument::JobAction::ActionInput::JSON_KEY_COMMANDS[];
constexpr char PlainJobDocument::JobAction::ActionInput::JSON_KEY_PATH[];

void PlainJobDocument::JobAction::ActionInput::LoadFromJobDocument(const JsonView &json)
{
    const char *jsonKey = JSON_KEY_HANDLER;
    if (json.ValueExists(jsonKey) && json.GetJsonObject(jsonKey).IsString())
    {
        handler = json.GetString(jsonKey).c_str();
    }

    jsonKey = JSON_KEY_ARGS;
    if (json.ValueExists(jsonKey) && json.GetJsonObject(jsonKey).IsListType())
    {
        args = ParseToVectorString(json.GetJsonObject(jsonKey));
    }

    jsonKey = JSON_KEY_COMMANDS;
    if (json.ValueExists(jsonKey) && json.GetJsonObject(jsonKey).IsListType())
    {
        commands = ParseToVectorString(json.GetJsonObject(jsonKey));
    }

    jsonKey = JSON_KEY_PATH;
    if (json.ValueExists(jsonKey) && json.GetJsonObject(jsonKey).IsString())
    {
        path = json.GetString(jsonKey).c_str();
    }
}

bool PlainJobDocument::JobAction::ActionInput::Validate() const
{
    if (currentType == ACTION_TYPE_RUN_HANDLER)
    {
        if (!handler.has_value() || handler->empty())
        {
            LOGM_ERROR(
                TAG, "*** %s: Required field ActionInput Handler is missing ***", DeviceClient::Jobs::DC_INVALID_JOB_DOC);
            return false;
        }
    }

    if (currentType == ACTION_TYPE_RUN_COMMAND)
    {
        if (!commands.has_value() || commands->empty())
        {
            LOGM_ERROR(
                TAG, "*** %s: Required field ActionInput commands is missing ***", DeviceClient::Jobs::DC_INVALID_JOB_DOC);
            return false;
        }
    }

    return true;
}
