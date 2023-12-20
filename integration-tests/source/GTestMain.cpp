// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "IntegrationTestResourceHandler.h"
#include "gtest/gtest.h"
#include <aws/core/Aws.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/iot/IoTClient.h>
#include <aws/iot/model/CreateJobRequest.h>
#include <fstream>
#include <thread>
#include <wordexp.h>

static constexpr char FLEET_PROVISIONING_RUNTIME_CONFIG_FILE[] =
    "~/.aws-iot-device-client/aws-iot-device-client-runtime.conf";

static constexpr char CLI_THING_NAME[] = "--thing-name";
static constexpr char CLI_REGION[] = "--region";
static constexpr char CLI_PORT[] = "--port";
static constexpr char CLI_LOCAL_PROXY_PATH[] = "--localproxy";
static constexpr char CLI_CLEAN_UP[] = "--clean-up";
static constexpr char CLI_SKIP_ST[] = "--skip-st";
static constexpr char CLI_HELP[] = "--help";

std::shared_ptr<IntegrationTestResourceHandler> resourceHandler;
std::string THING_NAME;
std::string REGION = "us-east-1";
std::string PORT = "5555";
std::string LOCAL_PROXY_PATH = "/localproxy";
bool CLEAN_UP = false;
bool SKIP_FP = false;
bool SKIP_ST = false;

std::string ExtractExpandedPath(const std::string &filePath)
{
    if (filePath.empty())
    {
        return "";
    }
    wordexp_t word;
    if (wordexp(filePath.c_str(), &word, 0))
    {
        printf("Failed to extract expanded file path for: %s \n", filePath.c_str());
        return "";
    }
    std::string expandedPath = word.we_wordv[0];
    wordfree(&word);
    return expandedPath;
}

std::string GetThingNameFromConfig()
{
    std::string thingName;
    std::ifstream runtimeConfig(ExtractExpandedPath(FLEET_PROVISIONING_RUNTIME_CONFIG_FILE));
    if (runtimeConfig.is_open())
    {
        std::string contents((std::istreambuf_iterator<char>(runtimeConfig)), std::istreambuf_iterator<char>());
        thingName =
            Aws::Utils::Json::JsonValue(contents).View().GetObject("runtime-config").GetObject("thing-name").AsString();
        runtimeConfig.close();
    }
    return thingName;
}

void PrintHelp()
{
    printf("\nAdditional options for the AWs IoT Device Client Integration Tests:\n");
    printf("--thing-name        Thing Group ARN to run the tests against\n");
    printf("--region            The AWS Region to run the tests. Example: us-east-1\n");
    printf("--port              The local port to run Local Proxy.\n");
    printf("--localproxy        Path to local proxy binary for Secure Tunneling tests.\n");
    printf("--skip-st           Skip Secure Tunneling integration tests\n");
    printf(
        "--clean-up          (Caution) Pass this flag kill to Device Client on the devices delete the provisioned IoT "
        "Things designated by --thing-name.\n");
    printf("--help              Print this message\n");
}

// GoogleTest modifies argv so not const
bool parseCliArgs(int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
    {
        std::string currentArg = argv[i];
        if (currentArg == CLI_THING_NAME)
        {
            THING_NAME = argv[++i];
        }
        else if (currentArg == CLI_REGION)
        {
            REGION = argv[++i];
        }
        else if (currentArg == CLI_PORT)
        {
            PORT = argv[++i];
        }
        else if (currentArg == CLI_LOCAL_PROXY_PATH)
        {
            LOCAL_PROXY_PATH = argv[++i];
        }
        else if (currentArg == CLI_SKIP_ST)
        {
            SKIP_ST = true;
        }
        else if (currentArg == CLI_CLEAN_UP)
        {
            CLEAN_UP = false;
        }
        else
        {
            if (currentArg != CLI_HELP)
            {
                printf("Failed to parse CLI args\n");
            }
            PrintHelp();
            return false;
        }
    }
    return true;
}
class GlobalEnvironment : public ::testing::Environment
{
  public:
    ~GlobalEnvironment() override {}

    // cppcheck-suppress unusedFunction
    void SetUp() override
    {
        Aws::Client::ClientConfiguration clientConfig;
        clientConfig.region = REGION;
        resourceHandler =
            std::unique_ptr<IntegrationTestResourceHandler>(new IntegrationTestResourceHandler(clientConfig));
    }

    // cppcheck-suppress unusedFunction
    void TearDown() override
    {
        if (CLEAN_UP)
        {
            printf("Clean up thingName: %s\n", THING_NAME.c_str());
            resourceHandler->CleanUpThingAndCert(THING_NAME);
        }
        else
        {
            printf("Skipping clean up for thingName: %s\n", THING_NAME.c_str());
            resourceHandler->GetTargetArn(THING_NAME);
        }
        resourceHandler.reset();
    }
};

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    try
    {
        parseCliArgs(argc, argv);
        if (THING_NAME.empty())
        {
            THING_NAME = GetThingNameFromConfig();
            if (THING_NAME.empty())
            {
                printf("No thing name was specified and could not parse from runtime config.\n");
                return 1;
            }
        }
        else
        {
            SKIP_FP = true;
        }
    }
    catch (const std::exception &e)
    {
        printf("%s\n", e.what());
    }
    catch (...)
    {
        printf("Unknown Exception while parsing test arguments\n");
    }

    Aws::SDKOptions options;
    Aws::InitAPI(options);
    ::testing::AddGlobalTestEnvironment(new GlobalEnvironment());
    int rc = RUN_ALL_TESTS();
    printf("Tests Complete!\n");
    Aws::ShutdownAPI(options);
    return rc;
}
