// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "SharedCrtResourceManager.h"
#include "Version.h"
#include "aws/crt/http/HttpProxyStrategy.h"
#include "logging/LoggerFactory.h"
#include "util/FileUtils.h"
#include "util/ProxyUtils.h"
#include "util/Retry.h"
#include "util/StringUtils.h"

#include <aws/crt/Api.h>
#include <aws/crt/io/Pkcs11.h>
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
constexpr char SharedCrtResourceManager::DEFAULT_SDK_LOG_FILE[];

SharedCrtResourceManager::~SharedCrtResourceManager()
{
    if (memTraceLevel != AWS_MEMTRACE_NONE)
    {
        allocator = aws_mem_tracer_destroy(allocator);
    }
}

bool SharedCrtResourceManager::initialize(
    const PlainConfig &config,
    std::shared_ptr<Util::FeatureRegistry> featureRegistry)
{
    features = featureRegistry;
    initialized = buildClient(config) == SharedCrtResourceManager::SUCCESS;
    return initialized;
}

void SharedCrtResourceManager::loadMemTraceLevelFromEnvironment()
{
    const char *memTraceLevelStr = std::getenv("AWS_CRT_MEMORY_TRACING");
    if (memTraceLevelStr)
    {
        switch (atoi(memTraceLevelStr))
        {
            case AWS_MEMTRACE_BYTES:
                LOG_DEBUG(Config::TAG, "Set AWS_CRT_MEMORY_TRACING=AWS_MEMTRACE_BYTES");
                memTraceLevel = AWS_MEMTRACE_BYTES;
                break;
            case AWS_MEMTRACE_STACKS:
                LOG_DEBUG(Config::TAG, "Set AWS_CRT_MEMORY_TRACING=AWS_MEMTRACE_STACKS");
                memTraceLevel = AWS_MEMTRACE_STACKS;
                break;
            default:
                break;
        }
    }
}

bool SharedCrtResourceManager::locateCredentials(const PlainConfig &config) const
{
    struct stat fileInfo;
    bool locatedAll = true;
    if (config.secureElement.enabled)
    {
        if (stat(config.secureElement.pkcs11Lib->c_str(), &fileInfo) != 0)
        {
            LOGM_ERROR(
                TAG,
                "Failed to find PKCS#11 library file: %s, cannot establish MQTT connection",
                Sanitize(config.secureElement.pkcs11Lib->c_str()).c_str());
            locatedAll = false;
        }
        else
        {
            string parentDir = FileUtils::ExtractParentDirectory(config.secureElement.pkcs11Lib->c_str());
            if (!FileUtils::ValidateFilePermissions(parentDir, Permissions::PKCS11_LIB_DIR) ||
                !FileUtils::ValidateFilePermissions(
                    config.secureElement.pkcs11Lib->c_str(), Permissions::PKCS11_LIB_FILE))
            {
                LOG_ERROR(TAG, "Incorrect permissions on PKCS#11 library file and/or it's parent directory");
                locatedAll = false;
            }
        }
    }
    else
    {
        if (stat(config.key->c_str(), &fileInfo) != 0)
        {
            LOGM_ERROR(
                TAG, "Failed to find %s, cannot establish MQTT connection", Sanitize(config.key->c_str()).c_str());
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

    return locatedAll;
}

bool SharedCrtResourceManager::setupLogging(const PlainConfig &config) const
{
    // Absolute path to the sdk log file.
    std::string logFilePath{DEFAULT_SDK_LOG_FILE};
    if (!config.logConfig.sdkLogFile.empty())
    {
        logFilePath = config.logConfig.sdkLogFile;
    }

    std::string logFileDir = FileUtils::ExtractParentDirectory(logFilePath);
    if (!FileUtils::DirectoryExists(logFileDir))
    {
        // Create an empty directory with the expected permissions.
        if (!FileUtils::CreateDirectoryWithPermissions(logFileDir.c_str(), S_IRWXU | S_IRGRP | S_IROTH | S_IXOTH))
        {
            return false;
        }
    }
    else
    {
        // Verify the directory permissions.
        auto rcvDirPermissions = FileUtils::GetFilePermissions(logFileDir);
        if (Permissions::LOG_DIR != rcvDirPermissions)
        {
            LOGM_ERROR(
                TAG,
                "Incorrect directory permissions for SDK log file: %s expected: %d received: %d",
                Sanitize(logFileDir).c_str(),
                Permissions::LOG_DIR,
                rcvDirPermissions);
            return false;
        }
    }

    if (!FileUtils::FileExists(logFilePath))
    {
        // Create an empty file with the expected permissions.
        if (!FileUtils::CreateEmptyFileWithPermissions(logFilePath, S_IRUSR | S_IWUSR))
        {
            return false;
        }
    }
    else
    {
        // Verify the file permissions.
        auto rcvFilePermissions = FileUtils::GetFilePermissions(logFilePath);
        if (Permissions::LOG_FILE != rcvFilePermissions)
        {
            LOGM_ERROR(
                TAG,
                "Incorrect file permissions for SDK log file: %s expected: %d received: %d",
                Sanitize(logFilePath).c_str(),
                Permissions::LOG_FILE,
                rcvFilePermissions);
            return false;
        }
    }

    // Configure the SDK with the log file path.
    apiHandle->InitializeLogging(config.logConfig.sdkLogLevel, logFilePath.c_str());
    LOGM_INFO(TAG, "SDK logging is enabled. Check %s for SDK logs.", Sanitize(logFilePath).c_str());

    return true;
}

void SharedCrtResourceManager::initializeAllocator()
{
    loadMemTraceLevelFromEnvironment();
    allocator = aws_default_allocator();

    if (memTraceLevel != AWS_MEMTRACE_NONE)
    {
        // If memTraceLevel == AWS_MEMTRACE_STACKS(2), then by default 8 frames per stack are used.
        allocator = aws_mem_tracer_new(allocator, nullptr, memTraceLevel, 0);
    }

    // We MUST declare an instance of the ApiHandle to perform global initialization
    // of the SDK libraries
    apiHandle = unique_ptr<ApiHandle>(new ApiHandle());
}

int SharedCrtResourceManager::buildClient(const PlainConfig &config)
{
    if (config.logConfig.sdkLoggingEnabled)
    {
        if (!setupLogging(config))
        {
            return SharedCrtResourceManager::ABORT;
        }
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
        // cppcheck-suppress nullPointerRedundantCheck
        LOGM_ERROR(
            TAG, "MQTT Event Loop Group Creation failed with error: %s", ErrorDebugString(eventLoopGroup->LastError()));
        // cppcheck-suppress nullPointerRedundantCheck
        return eventLoopGroup->LastError();
    }

    defaultHostResolver = unique_ptr<DefaultHostResolver>(new DefaultHostResolver(*eventLoopGroup, 2, 30));
    clientBootstrap = unique_ptr<ClientBootstrap>(new ClientBootstrap(*eventLoopGroup, *defaultHostResolver));

    if (!clientBootstrap)
    {
        // cppcheck-suppress nullPointerRedundantCheck
        LOGM_ERROR(TAG, "MQTT ClientBootstrap failed with error: %s", ErrorDebugString(clientBootstrap->LastError()));
        // cppcheck-suppress nullPointerRedundantCheck
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
    Aws::Iot::MqttClientConnectionConfigBuilder clientConfigBuilder;
    if (config.secureElement.enabled)
    {
        std::shared_ptr<Io::Pkcs11Lib> pkcs11Lib =
            Aws::Crt::Io::Pkcs11Lib::Create(config.secureElement.pkcs11Lib.value().c_str(), allocator);
        if (!pkcs11Lib)
        {
            LOGM_INFO(TAG, "Pkcs11Lib failed: %s", ErrorDebugString(Aws::Crt::LastError()));
            return ABORT;
        }

        Io::TlsContextPkcs11Options pkcs11Options(pkcs11Lib);
        pkcs11Options.SetCertificateFilePath(config.cert->c_str());
        pkcs11Options.SetUserPin(config.secureElement.secureElementPin->c_str());

        if (config.secureElement.secureElementTokenLabel.has_value() &&
            !config.secureElement.secureElementTokenLabel->empty())
        {
            pkcs11Options.SetTokenLabel(config.secureElement.secureElementTokenLabel->c_str());
        }

        if (config.secureElement.secureElementSlotId.has_value())
        {
            pkcs11Options.SetSlotId(config.secureElement.secureElementSlotId.value());
        }

        if (config.secureElement.secureElementKeyLabel.has_value() &&
            !config.secureElement.secureElementKeyLabel->empty())
        {
            pkcs11Options.SetPrivateKeyObjectLabel(config.secureElement.secureElementKeyLabel->c_str());
        }

        clientConfigBuilder = MqttClientConnectionConfigBuilder(pkcs11Options);
    }
    else
    {
        clientConfigBuilder = MqttClientConnectionConfigBuilder(config.cert->c_str(), config.key->c_str());
    }

    clientConfigBuilder.WithEndpoint(config.endpoint->c_str());
    if (config.rootCa.has_value() && !config.rootCa->empty())
    {
        clientConfigBuilder.WithCertificateAuthority(config.rootCa->c_str());
    }
    clientConfigBuilder.WithSdkName(SharedCrtResourceManager::BINARY_NAME);
    clientConfigBuilder.WithSdkVersion(DEVICE_CLIENT_VERSION);

    PlainConfig::HttpProxyConfig proxyConfig = config.httpProxyConfig;
    Aws::Crt::Http::HttpClientConnectionProxyOptions proxyOptions;

    if (proxyConfig.httpProxyEnabled)
    {
        proxyOptions.HostName = proxyConfig.proxyHost->c_str();
        proxyOptions.Port = proxyConfig.proxyPort.value();
        proxyOptions.ProxyConnectionType = Aws::Crt::Http::AwsHttpProxyConnectionType::Tunneling;

        LOGM_INFO(
            TAG,
            "Attempting to establish MQTT connection with proxy: %s:%u",
            proxyOptions.HostName.c_str(),
            proxyOptions.Port);

        if (proxyConfig.httpProxyAuthEnabled)
        {
            LOG_INFO(TAG, "Proxy Authentication is enabled");
            Aws::Crt::Http::HttpProxyStrategyBasicAuthConfig basicAuthConfig;
            basicAuthConfig.ConnectionType = Aws::Crt::Http::AwsHttpProxyConnectionType::Tunneling;
            proxyOptions.AuthType = Aws::Crt::Http::AwsHttpProxyAuthenticationType::Basic;
            basicAuthConfig.Username = proxyConfig.proxyUsername->c_str();
            basicAuthConfig.Password = proxyConfig.proxyPassword->c_str();
            proxyOptions.ProxyStrategy =
                Aws::Crt::Http::HttpProxyStrategy::CreateBasicHttpProxyStrategy(basicAuthConfig, Aws::Crt::g_allocator);
        }
        else
        {
            LOG_INFO(TAG, "Proxy Authentication is disabled");
            proxyOptions.AuthType = Aws::Crt::Http::AwsHttpProxyAuthenticationType::None;
        }

        clientConfigBuilder.WithHttpProxyOptions(proxyOptions);
    }

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
    connectionClosedPromise = std::promise<void>();

    /*
     * This will execute when an mqtt connect has completed or failed.
     */
    auto onConnectionCompleted = [this, &connectionCompletedPromise](
                                     const Mqtt::MqttConnection &, int errorCode, Mqtt::ReturnCode returnCode, bool) {
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
    auto onDisconnect = [this](const Mqtt::MqttConnection & /*conn*/) {
        {
            LOG_INFO(TAG, "MQTT Connection is now disconnected");
            connectionClosedPromise.set_value();
        }
    };

    /*
     * Invoked when connection is interrupted.
     */
    auto OnConnectionInterrupted = [this](const Mqtt::MqttConnection &, int errorCode) {
        {
            if (errorCode)
            {
                LOGM_ERROR(
                    TAG,
                    "MQTT Connection interrupted with error: `%s`. Device Client will retry connection until it is "
                    "successfully connected to the core. ",
                    ErrorDebugString(errorCode));
            }
        }
    };

    /*
     * Invoked when connection is resumed.
     */
    auto OnConnectionResumed = [this](const Mqtt::MqttConnection &, int returnCode, bool) {
        {
            LOGM_INFO(TAG, "MQTT connection resumed with return code: %d", returnCode);
        }
    };

    connection->OnConnectionCompleted = move(onConnectionCompleted);
    connection->OnDisconnect = move(onDisconnect);
    connection->OnConnectionInterrupted = move(OnConnectionInterrupted);
    connection->OnConnectionResumed = move(OnConnectionResumed);

    LOGM_INFO(TAG, "Establishing MQTT connection with client id %s...", config.thingName->c_str());
    if (!connection->SetReconnectTimeout(15, 240))
    {
        LOG_ERROR(TAG, "Device Client is not able to set reconnection settings. Device Client will retry again.");
        return RETRY;
    }
    if (!connection->Connect(config.thingName->c_str(), false))
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

aws_event_loop *SharedCrtResourceManager::getNextEventLoop()
{
    if (!initialized)
    {
        LOG_WARN(TAG, "Tried to get eventLoop but the SharedCrtResourceManager has not yet been initialized!");
        return nullptr;
    }

    return aws_event_loop_group_get_next_loop(eventLoopGroup->GetUnderlyingHandle());
}

aws_allocator *SharedCrtResourceManager::getAllocator()
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
    LOG_DEBUG(TAG, "Attempting to disconnect MQTT connection");
    if (connection == NULL)
    {
        return;
    }

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

void SharedCrtResourceManager::startDeviceClientFeatures() const
{
    LOG_INFO(TAG, "Starting Device Client features.");
    features->startAll();
}

void SharedCrtResourceManager::dumpMemTrace()
{
    if (memTraceLevel != AWS_MEMTRACE_NONE)
    {
        aws_mem_tracer_dump(allocator);
    }
}
