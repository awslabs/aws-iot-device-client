// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "SharedCrtResourceManager.h"
#include "logging/LoggerFactory.h"

#include <aws/crt/Api.h>
#include <sys/stat.h>

using namespace std;
using namespace Aws::Crt;
using namespace Aws::Crt::Io;
using namespace Aws::Crt::Mqtt;
using namespace Aws::Iot;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::Logging;

bool SharedCrtResourceManager::initialize(Aws::Crt::JsonView dcConfig)
{
    if (!locateCredentials(dcConfig))
    {
        LOG_ERROR(TAG, "Failed to find file(s) required for initializing the MQTT connection");
        return false;
    }

    initializeAllocator();
    initialized = buildClient() == SharedCrtResourceManager::SUCCESS &&
                  establishConnection(dcConfig) == SharedCrtResourceManager::SUCCESS;

    return initialized;
}

bool SharedCrtResourceManager::locateCredentials(Aws::Crt::JsonView dcConfig)
{
    struct stat fileInfo;
    bool locatedAll = true;
    if (stat(dcConfig.GetString(Config::PRIVATE_KEY).c_str(), &fileInfo) != 0)
    {
        LOGM_ERROR(
            TAG,
            "Failed to find %s, cannot establish MQTT connection",
            dcConfig.GetString(Config::PRIVATE_KEY).c_str());
        locatedAll = false;
    }

    if (stat(dcConfig.GetString(Config::CERTIFICATE).c_str(), &fileInfo) != 0)
    {
        LOGM_ERROR(
            TAG,
            "Failed to find %s, cannot establish MQTT connection",
            dcConfig.GetString(Config::CERTIFICATE).c_str());
        locatedAll = false;
    }

    if (stat(dcConfig.GetString(Config::ROOT_CA).c_str(), &fileInfo) != 0)
    {
        LOGM_ERROR(
            TAG, "Failed to find %s, cannot establish MQTT connection", dcConfig.GetString(Config::ROOT_CA).c_str());
        locatedAll = false;
    }

    return locatedAll;
}

void SharedCrtResourceManager::initializeAllocator()
{
    allocator = aws_mem_tracer_new(aws_default_allocator(), nullptr, AWS_MEMTRACE_BYTES, 0);
}

int SharedCrtResourceManager::buildClient()
{
    // We MUST declare an instance of the ApiHandle to perform global initialization
    // of the SDK libraries
    apiHandle = unique_ptr<ApiHandle>(new ApiHandle());

    eventLoopGroup = unique_ptr<EventLoopGroup>(
        new EventLoopGroup(1 // The number of threads used depends on your use-case. IF you have a maximum of less than
                             // a few hundred connections 1 thread is the ideal threadCount.
                           ));
    if (!eventLoopGroup)
    {
        LOGM_ERROR(
            TAG, "MQTT Event Loop Group Creation failed with error: %s", ErrorDebugString(eventLoopGroup->LastError()));
        return eventLoopGroup->LastError();
    }

    defaultHostResolver = unique_ptr<DefaultHostResolver>(new DefaultHostResolver(*eventLoopGroup, 2, 30));
    clientBootstrap = unique_ptr<ClientBootstrap>(new ClientBootstrap(*eventLoopGroup, *defaultHostResolver));

    if (!clientBootstrap)
    {
        LOGM_ERROR(TAG, "MQTT ClientBootstrap failed with error: %s", ErrorDebugString(clientBootstrap->LastError()));
        return clientBootstrap->LastError();
    }

    /*
     * Now Create a client. This can not throw.
     * An instance of a client must outlive its connections.
     * It is the users responsibility to make sure of this.
     */
    mqttClient = unique_ptr<MqttClient>(new MqttClient(*clientBootstrap));
    return SharedCrtResourceManager::SUCCESS;
}

int SharedCrtResourceManager::establishConnection(Aws::Crt::JsonView dcConfig)
{
    auto clientConfigBuilder = MqttClientConnectionConfigBuilder(
        dcConfig.GetString(Config::CERTIFICATE).c_str(), dcConfig.GetString(Config::PRIVATE_KEY).c_str());
    clientConfigBuilder.WithEndpoint(dcConfig.GetString(Config::ENDPOINT).c_str());
    clientConfigBuilder.WithCertificateAuthority(dcConfig.GetString(Config::ROOT_CA).c_str());
    auto clientConfig = clientConfigBuilder.Build();

    if (!clientConfig)
    {
        LOGM_ERROR(
            TAG,
            "MQTT Client Configuration initialization failed with error: %s",
            ErrorDebugString(clientConfig.LastError()));
        return clientConfig.LastError();
    }

    connection = mqttClient->NewConnection(clientConfig);

    if (!*connection)
    {
        LOGM_ERROR(TAG, "MQTT Connection Creation failed with error: %s", ErrorDebugString(connection->LastError()));
        return connection->LastError();
    }

    /*
     * TODO: Look into if anything needs changed with this synchronous behavior
     *
     * In a real world application you probably don't want to enforce synchronous behavior
     * but this is a sample console application, so we'll just do that with a condition variable.
     */
    promise<bool> connectionCompletedPromise;
    promise<void> connectionClosedPromise;

    /*
     * This will execute when an mqtt connect has completed or failed.
     */
    auto onConnectionCompleted = [&](Mqtt::MqttConnection &, int errorCode, Mqtt::ReturnCode returnCode, bool) {
        if (errorCode)
        {
            LOGM_ERROR(TAG, "MQTT Connection failed with error: %s", ErrorDebugString(errorCode));
            if (AWS_ERROR_MQTT_UNEXPECTED_HANGUP == errorCode)
            {
                LOG_ERROR(TAG, "*** Did you make sure to attach iot:* to your policy for this thing?");
                LOG_ERROR(TAG, "*** AWS Console -> IoT Core -> Secure -> Certificates");
            }
            connectionCompletedPromise.set_value(false);
        }
        else
        {
            LOGM_INFO(TAG, "MQTT connection established with return code: %d", returnCode);
            connectionCompletedPromise.set_value(true);
        }
    };

    /*
     * Invoked when a disconnect message has completed.
     */
    auto onDisconnect = [&](Mqtt::MqttConnection & /*conn*/) {
        {
            LOG_INFO(TAG, "MQTT Connection is now disconnected");
            connectionClosedPromise.set_value();
        }
    };

    connection->OnConnectionCompleted = move(onConnectionCompleted);
    connection->OnDisconnect = move(onDisconnect);

    /*
     * Actually perform the connect dance.
     */
    LOGM_INFO(TAG, "Establishing MQTT connection with client id %s...", dcConfig.GetString(Config::THING_NAME).c_str());
    if (!connection->Connect(dcConfig.GetString(Config::THING_NAME).c_str(), true, 0))
    {
        LOGM_ERROR(TAG, "MQTT Connection failed with error: %s", ErrorDebugString(connection->LastError()));
        return connection->LastError();
    }

    if (connectionCompletedPromise.get_future().get())
    {
        LOG_INFO(TAG, "Shared MQTT connection is ready!");
    }
    else
    {
        LOG_ERROR(TAG, "Failed to establish shared MQTT connection! ***");
        return SharedCrtResourceManager::ABORT;
    }

    return SharedCrtResourceManager::SUCCESS;
}

shared_ptr<MqttConnection> SharedCrtResourceManager::getConnection()
{
    if (!initialized)
    {
        LOG_WARN(TAG, "Tried to get connection but the SharedCrtResourceManager has not yet been initialized!");
        return nullptr;
    }

    return connection;
}

EventLoopGroup *SharedCrtResourceManager::getEventLoopGroup()
{
    if (!initialized)
    {
        LOG_WARN(TAG, "Tried to get eventLoopGroup but the SharedCrtResourceManager has not yet been initialized!");
        return nullptr;
    }

    return eventLoopGroup.get();
}

struct aws_allocator *SharedCrtResourceManager::getAllocator()
{
    if (!initialized)
    {
        LOG_WARN(TAG, "Tried to get allocator but the SharedCrtResourceManager has not yet been initialized!");
        return nullptr;
    }

    return allocator;
}

Aws::Crt::Io::ClientBootstrap *SharedCrtResourceManager::getClientBootstrap()
{
    if (!initialized)
    {
        LOG_WARN(TAG, "Tried to get clientBootstrap but the SharedCrtResourceManager has not yet been initialized!");
        return nullptr;
    }

    return clientBootstrap.get();
}

void SharedCrtResourceManager::disconnect()
{
    if (connection->Disconnect())
    {
        initialized = false;
    }
    else
    {
        LOG_ERROR(TAG, "MQTT Connection failed to disconnect");
    }
}
