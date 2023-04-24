// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../IntegrationTestResourceHandler.h"
#include <aws/iot/model/ListThingsInThingGroupRequest.h>
#include <aws/iotsecuretunneling/IoTSecureTunnelingClient.h>
#include <aws/iotsecuretunneling/model/ConnectionStatus.h>
#include <aws/iotsecuretunneling/model/OpenTunnelResult.h>
#include <gtest/gtest.h>
#include <thread>

using namespace Aws;
using namespace Aws::Utils;
using namespace Aws::Auth;
using namespace Aws::Client;
using namespace Aws::IoT;
using namespace Aws::IoT::Model;
using namespace std;

extern string THING_NAME;
extern string PORT;
extern string REGION;
extern bool SKIP_ST;
extern string LOCAL_PROXY_PATH;
extern std::shared_ptr<IntegrationTestResourceHandler> resourceHandler;

const string TEST_TUNNEL_PATH = "/test-tunnel.sh";

class TestSecureTunnelingFeature : public ::testing::Test
{
  public:
    void SetUp() override
    {
        if (!SKIP_ST)
        {

            Aws::IoTSecureTunneling::Model::OpenTunnelResult openTunnelResult = resourceHandler->OpenTunnel(THING_NAME);
            tunnelId = openTunnelResult.GetTunnelId();
            sourceToken = openTunnelResult.GetSourceAccessToken();

            // cppcheck-suppress leakReturnValNotUsed
            std::unique_ptr<const char *[]> argv(new const char *[8]);
            argv[0] = LOCAL_PROXY_PATH.c_str();
            argv[1] = "-s";
            argv[2] = PORT.c_str();
            argv[3] = "-r";
            argv[4] = REGION.c_str();
            argv[5] = "-t";
            argv[6] = sourceToken.c_str();
            argv[7] = nullptr;

            PID = fork();
            if (PID == 0)
            {
                printf("Started Child Process to run Local Proxy\n");
                if (execvp(LOCAL_PROXY_PATH.c_str(), const_cast<char *const *>(argv.get())) == -1)
                {
                    printf("Failed to initialize Local Proxy.\n");
                }
            }
        }
        else
        {
            printf("Skipping Secure Tunneling Tests\n");
            GTEST_SKIP();
        }
    }
    void TearDown() override
    {
        if (PID == 0)
        {
            _exit(0);
        }
        resourceHandler->CleanUp();
    }
    string tunnelId;
    string sourceToken;
    int PID;
};

TEST_F(TestSecureTunnelingFeature, SCP)
{
    if (resourceHandler->GetTunnelSourceConnectionStatusWithRetry(tunnelId) !=
        Aws::IoTSecureTunneling::Model::ConnectionStatus::CONNECTED)
    {
        printf("Tunnel Source Failed to connect\n");
        GTEST_FAIL();
    }
    printf("Running %s script...\n", TEST_TUNNEL_PATH.c_str());

    // cppcheck-suppress leakReturnValNotUsed
    std::unique_ptr<const char *[]> argv(new const char *[3]);
    argv[0] = TEST_TUNNEL_PATH.c_str();
    argv[1] = PORT.c_str();
    argv[2] = nullptr;
    int execResult;
    int pid = fork();
    if (pid == 0)
    {
        if (execvp(TEST_TUNNEL_PATH.c_str(), const_cast<char *const *>(argv.get())) == -1)
        {
            printf("%s failed", TEST_TUNNEL_PATH.c_str());
            _exit(1);
        }
    }
    else
    {
        int waitReturn = waitpid(pid, &execResult, 0);
        if (waitReturn == -1)
        {
            GTEST_FAIL();
        }
    }
    ASSERT_EQ(execResult, 0);
}
