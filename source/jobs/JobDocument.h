// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef AWS_IOT_DEVICE_CLIENT_JOBDOCUMENT_H
#define AWS_IOT_DEVICE_CLIENT_JOBDOCUMENT_H

#include <aws/crt/Api.h>
#include <aws/crt/JsonObject.h>
#include <aws/crt/Optional.h>
#include <map>

using namespace Aws::Crt;

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Jobs
            {
                static constexpr char DC_INVALID_JOB_DOC[] = "AWS IOT DEVICE CLIENT RECEIVED INVALID JOB DOC";

                class LoadableFromJobDocument
                {
                  public:
                    virtual void LoadFromJobDocument(const Crt::JsonView &json) = 0;
                    virtual bool Validate() const = 0;
                    virtual ~LoadableFromJobDocument() = default;

                    static constexpr char TAG[] = "JobDocument.cpp";
                };

                struct PlainJobDocument : public LoadableFromJobDocument
                {
                    void LoadFromJobDocument(const Crt::JsonView &json) override;
                    bool Validate() const override;

                    static constexpr char ACTION_TYPE_RUN_HANDLER[] = "runHandler";
                    static constexpr char ACTION_TYPE_RUN_COMMAND[] = "runCommand";

                    static constexpr char JSON_KEY_VERSION[] = "version";
                    static constexpr char JSON_KEY_INCLUDESTDOUT[] = "includeStdOut";
                    static constexpr char JSON_KEY_CONDITIONS[] = "conditions";
                    static constexpr char JSON_KEY_STEPS[] = "steps";
                    static constexpr char JSON_KEY_ACTION[] = "action";
                    static constexpr char JSON_KEY_FINALSTEP[] = "finalStep";

                    // Old Schema Fields
                    static constexpr char JSON_KEY_OPERATION[] = "operation";
                    static constexpr char JSON_KEY_ARGS[] = "args";
                    static constexpr char JSON_KEY_ALLOWSTDERR[] = "allowStdErr";
                    static constexpr char JSON_KEY_PATH[] = "path";

                    static constexpr char OLD_SCHEMA_VERSION[] = "0.0";

                    std::string version;
                    Crt::Optional<bool> includeStdOut{false};

                    struct JobCondition : public LoadableFromJobDocument
                    {
                        void LoadFromJobDocument(const Crt::JsonView &json) override;
                        bool Validate() const override;

                        static constexpr char JSON_KEY_CONDITION_KEY[] = "key";
                        static constexpr char JSON_KEY_CONDITION_VALUE[] = "value";
                        static constexpr char JSON_KEY_TYPE[] = "type";

                        std::string conditionKey;
                        std::vector<std::string> conditionValue;
                        Crt::Optional<std::string> type{"stringEqual"};
                    };
                    Crt::Optional<std::vector<JobCondition>> conditions;

                    struct JobAction : public LoadableFromJobDocument
                    {
                        void LoadFromJobDocument(const Crt::JsonView &json) override;
                        bool Validate() const override;

                        static constexpr char JSON_KEY_NAME[] = "name";
                        static constexpr char JSON_KEY_TYPE[] = "type";
                        static constexpr char JSON_KEY_INPUT[] = "input";
                        static constexpr char JSON_KEY_RUNASUSER[] = "runAsUser";
                        static constexpr char JSON_KEY_ALLOWSTDERR[] = "allowStdErr";
                        static constexpr char JSON_KEY_IGNORESTEPFAILURE[] = "ignoreStepFailure";

                        std::string name;
                        std::string type;

                        /**
                         * ActionHandlerInput - Invokes a handler script specified in a job document.
                         */
                        struct ActionHandlerInput : public LoadableFromJobDocument
                        {
                            void LoadFromJobDocument(const Crt::JsonView &json) override;
                            bool Validate() const override;

                            static constexpr char JSON_KEY_HANDLER[] = "handler";
                            static constexpr char JSON_KEY_ARGS[] = "args";
                            static constexpr char JSON_KEY_PATH[] = "path";

                            std::string handler;
                            Crt::Optional<std::vector<std::string>> args;
                            Crt::Optional<std::string> path;
                        };
                        Optional<ActionHandlerInput> handlerInput;

                        /**
                         * ActionCommandInput - Invokes arbitrary commands specified in a job document.
                         */
                        struct ActionCommandInput : public LoadableFromJobDocument
                        {
                            void LoadFromJobDocument(const JsonView &json) override;
                            bool Validate() const override;

                            static constexpr char JSON_KEY_COMMAND[] = "command";

                            std::vector<std::string> command;
                        };
                        Optional<ActionCommandInput> commandInput;
                        Optional<std::string> runAsUser{""};
                        Optional<int> allowStdErr;
                        Optional<bool> ignoreStepFailure{false};
                    };
                    std::vector<JobAction> steps;

                    Crt::Optional<JobAction> finalStep;
                };

            } // namespace Jobs
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // AWS_IOT_DEVICE_CLIENT_JOBDOCUMENT_H
