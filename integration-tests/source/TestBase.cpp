// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "TestBase.h"

void TestBase::init()
{
    options.ioOptions.clientBootstrap_create_fn = [] {
        Aws::Crt::Io::EventLoopGroup eventLoopGroup(1);
        Aws::Crt::Io::DefaultHostResolver defaultHostResolver(eventLoopGroup, 8, 30);
        return Aws::MakeShared<Aws::Crt::Io::ClientBootstrap>("Aws_Init_Cleanup", eventLoopGroup, defaultHostResolver);
    };
    Aws::InitAPI(options);
}