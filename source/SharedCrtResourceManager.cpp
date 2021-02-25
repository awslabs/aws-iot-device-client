// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "SharedCrtResourceManager.h"
#include "logging/LoggerFactory.h"
#include "util/FileUtils.h"
#include "util/Retry.h"
#include "util/StringUtils.h"

#include <aws/crt/Api.h>
#include <sys/stat.h>

using namespace std;
using namespace Aws::Crt;
using namespace Aws::Crt::Io;
using namespace Aws::Crt::Mqtt;
using namespace Aws::Iot;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::Util;
using namespace Aws::Iot::DeviceClient::Logging;

constexpr int SharedCrtResourceManager::DEFAULT_WAIT_TIME_SECONDS;

bool SharedCrtResourceManager::initialize(const PlainConfig &config)
{
    initializeAllocator();
    initialized = buildClient(config) == SharedCrtResourceManager::SUCCESS;
    return initialized;
}

bool SharedCrtResourceManager::locateCredentials(const PlainConfig &config)
{
    struct stat fileInfo;
    bool locatedAll = true;
    if (stat(config.key->c_str(), &fileInfo) != 0)
    {
        LOGM_ERROR(TAG, "Failed to find %s, cannot establish MQTT connection", Sanitize(config.key->c_str()).c_str());
        locatedAll = false;
    }
    else
    {
        string parentDir = FileUtils::ExtractParentDirectory(config.key->c_str());
        if (!FileUtils::ValidateFilePermissions(parentDir, Permissions::KEY_DIR) ||
            !FileUtils::ValidateFilePermissions(config.key->c_str(), Permissions::PRIVATE_KEY))
        {
            LOG_ERROR(TAG, "Incorrect permissions on private key file and/or parent directory");
            locatedAll = false;
        }
    }

    if (stat(config.cert->c_str(), &fileInfo) != 0)
    {
        LOGM_ERROR(TAG, "Failed to find %s, cannot establish MQTT connection", Sanitize(config.cert->c_str()).c_str());
        locatedAll = false;
    }
    else
    {
        string parentDir = FileUtils::ExtractParentDirectory(config.cert->c_str());
        if (!FileUtils::ValidateFilePermissions(parentDir, Permissions::CERT_DIR) ||
            !FileUtils::ValidateFilePermissions(config.cert->c_str(), Permissions::PUBLIC_CERT))
        {
            LOG_ERROR(TAG, "Incorrect permissions on public cert file and/or parent directory");
            locatedAll = false;
        }
    }

    if (stat(config.rootCa->c_str(), &fileInfo) != 0)
    {
        LOGM_ERROR(
            TAG, "Failed to find %s, cannot establish MQTT connection", Sanitize(config.rootCa->c_str()).c_str());
        locatedAll = false;
    }
    else
    {
        string parentDir = FileUtils::ExtractParentDirectory(config.rootCa->c_str());
        if (!FileUtils::ValidateFilePermissions(parentDir, Permissions::ROOT_CA_DIR) ||
            !FileUtils::ValidateFilePermissions(config.rootCa->c_str(), Permissions::ROOT_CA))
        {
            LOG_ERROR(TAG, "Incorrect permissions on Root CA file and/or parent directory");
            locatedAll = false;
        }
    }

    return locatedAll;
}

void SharedCrtResourceManager::initializeAllocator()
{
    allocator = aws_mem_tracer_new(aws_default_allocator(), nullptr, AWS_MEMTRACE_BYTES, 0);
}

int SharedCrtResourceManager::buildClient(const PlainConfig &config)
{
    // We MUST declare an instance of the ApiHandle to perform global initialization
    // of the SDK libraries
    apiHandle = unique_ptr<ApiHandle>(new ApiHandle());
    if (config.logConfig.sdkLoggingEnabled)
    {
        apiHandle->InitializeLogging(config.logConfig.sdkLogLevel, config.logConfig.sdkLogFile.c_str());
        LOGM_INFO(TAG, "SDK logging is enabled. Check %s for SDK logs.", Sanitize(config.logConfig.sdkLogFile).c_str());
    }
    else
    {
        LOG_INFO(
            TAG,
            "SDK logging is disabled. Enable it with --enable-sdk-logging on the command line or "
            "logging::enable-sdk-logging in your configuration file");
    }

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

void SharedCrtResourceManager::initializeAWSHttpLib()
{
    if (!initialized)
    {
        LOG_WARN(TAG, "Tried to aws_http_library_init but the SharedCrtResourceManager has not yet been initialized!");
        return;
    }
    if (initializedAWSHttpLib)
    {
        LOG_WARN(TAG, "Tried to aws_http_library_init but it was already initialized!");
        return;
    }
    aws_http_library_init(getAllocator());
    initializedAWSHttpLib = true;
}

int SharedCrtResourceManager::establishConnection(const PlainConfig &config)
{
    if (!locateCredentials(config))
    {
        LOGM_ERROR(
            TAG,
            "*** %s: Failed to find file(s) with correct permissions required for establishing the MQTT connection ***",
            DeviceClient::DC_FATAL_ERROR);
        return SharedCrtResourceManager::ABORT;
    }
    auto clientConfigBuilder = MqttClientConnectionConfigBuilder(config.cert->c_str(), config.key->c_str());
    clientConfigBuilder.WithEndpoint(config.endpoint->c_str());
    clientConfigBuilder.WithCertificateAuthority(config.rootCa->c_str());
    auto clientConfig = clientConfigBuilder.Build();

    if (!clientConfig)
    {
        LOGM_ERROR(
            TAG,
            "MQTT Client Configuration initialization failed with error: %s",
            ErrorDebugString(clientConfig.LastError()));
        return ABORT;
    }

    connection = mqttClient->NewConnection(clientConfig);

    if (!*connection)
    {
        LOGM_ERROR(TAG, "MQTT Connection Creation failed with error: %s", ErrorDebugString(connection->LastError()));
        return ABORT;
    }

    promise<int> connectionCompletedPromise;

    /*
     * This will execute when an mqtt connect has completed or failed.
     */
    auto onConnectionCompleted = [&](Mqtt::MqttConnection &, int errorCode, Mqtt::ReturnCode returnCode, bool) {
        if (errorCode)
        {
            LOGM_ERROR(TAG, "MQTT Connection failed with error: %s", ErrorDebugString(errorCode));
            if (AWS_ERROR_MQTT_UNEXPECTED_HANGUP == errorCode)
            {
                LOG_ERROR(
                    TAG,
                    "*** Did you make sure you are using valid certificate with recommended policy attached to it? "
                    "Please refer README->Fleet Provisioning Feature section for more details on recommended policies "
                    "for AWS IoT Device Client. ***");
            }
            connectionCompletedPromise.set_value(errorCode);
        }
        else
        {
            LOGM_INFO(TAG, "MQTT connection established with return code: %d", returnCode);
            connectionCompletedPromise.set_value(0);
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

    LOGM_INFO(TAG, "Establishing MQTT connection with client id %s...", config.thingName->c_str());
    if (!connection->Connect(config.thingName->c_str(), true, 0))
    {
        LOGM_ERROR(TAG, "MQTT Connection failed with error: %s", ErrorDebugString(connection->LastError()));
        return RETRY;
    }

    int connectionStatus = connectionCompletedPromise.get_future().get();

    if (SharedCrtResourceManager::SUCCESS == connectionStatus)
    {
        LOG_INFO(TAG, "Shared MQTT connection is ready!");
        return SharedCrtResourceManager::SUCCESS;
    }
    else
    {
        LOG_ERROR(TAG, "Failed to establish shared MQTT connection, but will attempt retry...");
        return SharedCrtResourceManager::RETRY;
    }
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
        if (connectionClosedPromise.get_future().wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
            future_status::timeout)
        {
            LOG_ERROR(TAG, "MQTT Connection timed out to disconnect.");
        }
    }
    else
    {
        LOG_ERROR(TAG, "MQTT Connection failed to disconnect");
    }
}
