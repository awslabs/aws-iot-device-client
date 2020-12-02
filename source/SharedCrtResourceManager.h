// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_SHAREDCRTRESOURCEMANAGER_H
#define DEVICE_CLIENT_SHAREDCRTRESOURCEMANAGER_H

#include "config/Config.h"

#include <aws/iot/MqttClient.h>
#include <aws/crt/Api.h>
#include <sys/stat.h>
#include <iostream>

namespace Aws {
    namespace Iot {
        namespace DeviceClient {
            class SharedCrtResourceManager {
                private:
                    const char * TAG = "SharedCrtResourceManager.cpp";

                    bool initialized = false;
                    std::unique_ptr<Aws::Crt::ApiHandle> apiHandle;
                    std::unique_ptr<Aws::Crt::Io::EventLoopGroup> eventLoopGroup;
                    std::unique_ptr<Aws::Crt::Io::DefaultHostResolver> defaultHostResolver;
                    std::unique_ptr<Aws::Crt::Io::ClientBootstrap> clientBootstrap;
                    std::unique_ptr<Aws::Iot::MqttClient> mqttClient;
                    std::shared_ptr<Crt::Mqtt::MqttConnection> connection;
                    struct aws_allocator* allocator;

                    bool locateCredentials(const PlainConfig &config);
                    int buildClient();
                    void initializeAllocator();
                    int establishConnection(const PlainConfig &config);
                public:
                    static const int SUCCESS = 0;
                    static const int ABORT = 2;
                    bool initialize(const PlainConfig &config);
                    std::shared_ptr<Crt::Mqtt::MqttConnection> getConnection();
                    Aws::Crt::Io::EventLoopGroup* getEventLoopGroup();
                    struct aws_allocator* getAllocator();
                    Aws::Crt::Io::ClientBootstrap* getClientBootstrap();
                    void disconnect();
            };
        }
    }
}

#endif //DEVICE_CLIENT_SHAREDCRTRESOURCEMANAGER_H
