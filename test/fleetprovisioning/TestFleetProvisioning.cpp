// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/fleetprovisioning/FleetProvisioning.h"
#include "gtest/gtest.h"
#include <aws/crt/JsonObject.h>
#include <stdlib.h>

using namespace std;
using namespace Aws::Iot::DeviceClient::FleetProvisioningNS;

class FleetProvisioningFeature : public ::testing::Test
{
  public:
    FleetProvisioningFeature() = default;
    shared_ptr<Aws::Iot::DeviceClient::SharedCrtResourceManager> resourceManager;

    void SetUp() override
    {
        resourceManager = std::make_shared<Aws::Iot::DeviceClient::SharedCrtResourceManager>();

        Aws::Iot::DeviceClient::PlainConfig configuration;
        configuration.LoadMemTraceLevelFromEnvironment();
        resourceManager.get()->initializeAllocator(configuration.memTraceLevel);

    }
};

TEST_F(FleetProvisioningFeature, EmptyTemplateParameters)
{
    Aws::Crt::Optional<std::string> params; // start with empty value
    FleetProvisioning fp;
    ASSERT_TRUE(fp.MapParameters(params));
    params = "{}"; // test empty JSON
}

TEST_F(FleetProvisioningFeature, MalformedTemplateParameters)
{
    Aws::Crt::Optional<std::string> params("{\"SerialNumber\" \"Device-SN\"}"); // test missing colon
    FleetProvisioning fp;
    ASSERT_FALSE(fp.MapParameters(params));
    params = "{\"SerialNumber\": \"Device-SN\" \"ThingName\": \"MyDevice\"}"; // test more complex JSON. Missing comma
    ASSERT_FALSE(fp.MapParameters(params));
    params = ""; // test empty string (invalid JSON)
    ASSERT_FALSE(fp.MapParameters(params));
}

TEST_F(FleetProvisioningFeature, ValidTemplateParameters)
{
    Aws::Crt::Optional<std::string> params("{\"SerialNumber\": \"Device-SN\"}"); // test single JSON property
    FleetProvisioning fp;
    ASSERT_TRUE(fp.MapParameters(params));
    params = "{\"SerialNumber\": \"Device-SN\", \"ThingName\": \"MyDevice\"}"; // test more complex JSON
    ASSERT_TRUE(fp.MapParameters(params));
}
