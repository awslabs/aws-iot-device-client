// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef AWS_IOT_DEVICE_CLIENT_INTEGRATION_TESTS_TESTBASE_H
#define AWS_IOT_DEVICE_CLIENT_INTEGRATION_TESTS_TESTBASE_H

#include "IntegrationTestResourceHandler.h"
#include <aws/core/Aws.h>
#include <gtest/gtest.h>

class TestBase : public ::testing::Test
{

  protected:
    void init();
    std::unique_ptr<IntegrationTestResourceHandler> resourceHandler;

  private:
    Aws::SDKOptions options;
};

#endif // AWS_IOT_DEVICE_CLIENT_INTEGRATION_TESTS_TESTBASE_H
