// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/fleetprovisioning/FleetProvisioning.h"
#include "gtest/gtest.h"
#include <aws/crt/JsonObject.h>
#include <stdlib.h>

using namespace std;
using namespace Aws::Iot::DeviceClient::FleetProvisioningNS;

TEST(FleetProvisioning, EmptyTemplateParameters)
{
    Aws::Iot::DeviceClient::SharedCrtResourceManager resourceManager;
    Aws::Crt::Optional<std::string> params; // start with empty value
    FleetProvisioning fp;
    ASSERT_TRUE(fp.MapParameters(params));
    params = "{}"; // test empty JSON
}

TEST(FleetProvisioning, MalformedTemplateParameters)
{
    Aws::Iot::DeviceClient::SharedCrtResourceManager resourceManager;
    // Initializing allocator, so we can use CJSON lib from SDK in our unit tests.
    resourceManager.initializeAllocator();

    Aws::Crt::Optional<std::string> params("{\"SerialNumber\" \"Device-SN\"}"); // test missing colon
    FleetProvisioning fp;
    ASSERT_FALSE(fp.MapParameters(params));
    params = "{\"SerialNumber\": \"Device-SN\" \"ThingName\": \"MyDevice\"}"; // test more complex JSON. Missing comma
    ASSERT_FALSE(fp.MapParameters(params));
    params = ""; // test empty string (invalid JSON)
    ASSERT_FALSE(fp.MapParameters(params));
}

TEST(FleetProvisioning, ValidTemplateParameters)
{
    Aws::Iot::DeviceClient::SharedCrtResourceManager resourceManager;
    // Initializing allocator, so we can use CJSON lib from SDK in our unit tests.
    resourceManager.initializeAllocator();

    Aws::Crt::Optional<std::string> params("{\"SerialNumber\": \"Device-SN\"}"); // test single JSON property
    FleetProvisioning fp;
    ASSERT_TRUE(fp.MapParameters(params));
    params = "{\"SerialNumber\": \"Device-SN\", \"ThingName\": \"MyDevice\"}"; // test more complex JSON
    ASSERT_TRUE(fp.MapParameters(params));
}
