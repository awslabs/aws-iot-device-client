// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "ClientBaseNotification.h"
#include "Feature.h"
#include "SharedCrtResourceManager.h"
#include "Version.h"
#include "config/Config.h"
#include "util/EnvUtils.h"
#include "util/LockFile.h"
#include "util/Retry.h"

#if !defined(EXCLUDE_DD)

#    include "devicedefender/DeviceDefenderFeature.h"

#endif
#if !defined(EXCLUDE_JOBS)

#    include "jobs/JobsFeature.h"

#endif
#if !defined(EXCLUDE_FP)

#    include "fleetprovisioning/FleetProvisioning.h"

#endif

#include "logging/LoggerFactory.h"

#if !defined(EXCLUDE_ST)

#    include "tunneling/SecureTunnelingFeature.h"

#endif
#if !defined(EXCLUDE_SAMPLES)
#    if !defined(EXCLUDE_PUBSUB)

#        include "samples/pubsub/PubSubFeature.h"

#    endif
#endif
#if !defined(EXCLUDE_SHADOW)
#    if !defined(EXCLUDE_CONFIG_SHADOW)

#        include "shadow/ConfigShadow.h"

#    endif
#    if !defined(EXCLUDE_SAMPLE_SHADOW)

#        include "shadow/SampleShadowFeature.h"

#    endif
#endif

#if !defined(EXCLUDE_SENSOR_PUBLISH)

#    include "sensor-publish/SensorPublishFeature.h"

#endif

#include <csignal>
#include <memory>
#include <thread>
#include <vector>

using namespace std;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::Util;
#if !defined(EXCLUDE_DD)
using namespace Aws::Iot::DeviceClient::DeviceDefender;
#endif
#if !defined(EXCLUDE_JOBS)
using namespace Aws::Iot::DeviceClient::Jobs;
#endif
using namespace Aws::Iot::DeviceClient::Logging;
#if !defined(EXCLUDE_ST)
using namespace Aws::Iot::DeviceClient::SecureTunneling;
#endif
#if !defined(EXCLUDE_FP)
using namespace Aws::Iot::DeviceClient::FleetProvisioningNS;
#endif
#if !defined(EXCLUDE_SAMPLES)
#    if !defined(EXCLUDE_PUBSUB)
using namespace Aws::Iot::DeviceClient::Samples;
#    endif
#endif
#if !defined(EXCLUDE_SHADOW)
using namespace Aws::Iot::DeviceClient::Shadow;
#endif
#if !defined(EXCLUDE_SENSOR_PUBLISH)
using namespace Aws::Iot::DeviceClient::SensorPublish;
#endif

const char *TAG = "Main.cpp";

vector<Feature *> features;
shared_ptr<SharedCrtResourceManager> resourceManager;
unique_ptr<LockFile> lockFile;
mutex featuresReadWriteLock;
bool attemptingShutdown{false};
Config config;

/**
 * TODO: For future expandability of main
 * Currently creates a lockfile to prevent the creation of multiple Device Client processes.
 * @return true if no exception is caught, false otherwise
 */
bool init(int argc, char *argv[])
{
    try
    {
        string filename = config.config.lockFilePath;
        if (!filename.empty())
        {
            lockFile = unique_ptr<LockFile>(new LockFile{filename, argv[0]});
        }
    }
    catch (std::runtime_error &e)
    {
        LOGM_ERROR(TAG, "*** %s: Error obtaining lockfile: %s", DC_FATAL_ERROR, e.what());
        LoggerFactory::getLoggerInstance().get()->shutdown();
        return false;
    }
    return true;
}

/**
 * Attempts to perform a graceful shutdown of each running feature. If this function is
 * executed more than once, it will terminate immediately.
 */
void shutdown()
{
    featuresReadWriteLock.lock(); // LOCK
    // Make a copy of the features vector for thread safety
    vector<Feature *> featuresCopy = features;
    featuresReadWriteLock.unlock(); // UNLOCK

    if (!attemptingShutdown && !featuresCopy.empty())
    {
        attemptingShutdown = true;

        for (auto &feature : featuresCopy)
        {
            LOGM_DEBUG(TAG, "Attempting shutdown of %s", feature->getName().c_str());
            feature->stop();
        }

        resourceManager->dumpMemTrace();
    }
    else
    {
// terminate program
#if !defined(DISABLE_MQTT)
        resourceManager.get()->disconnect();
#endif
        LoggerFactory::getLoggerInstance().get()->shutdown();
        exit(0);
    }
}

/**
 * \brief This function shuts down device client when aborting execution due to some type of configuration issue
 *
 * @param reason the reason why the abort is happening
 */
void deviceClientAbort(const string &reason)
{
    cout << "AWS IoT Device Client must abort execution, reason: " << reason << endl;
    cout << "Please check the AWS IoT Device Client logs for more information" << endl;
    exit(EXIT_FAILURE);
}

void handle_feature_stopped(const Feature *feature)
{
    featuresReadWriteLock.lock(); // LOCK

    for (int i = 0; (unsigned)i < features.size(); i++)
    {
        if (features.at(i) == feature)
        {
            // Performing bookkeeping so we know when all features have stopped
            // and the entire program can be shutdown
            features.erase(features.begin() + i);
        }
    }

    const int size = features.size();
    featuresReadWriteLock.unlock(); // UNLOCK

    if (0 == size)
    {
        LOG_INFO(TAG, "All features have stopped");
        shutdown();
    }
}

void attemptConnection()
{
    Retry::ExponentialRetryConfig retryConfig = {10 * 1000, 900 * 1000, -1, nullptr};
    auto publishLambda = []() -> bool {
        int connectionStatus = resourceManager.get()->establishConnection(config.config);
        if (SharedCrtResourceManager::ABORT == connectionStatus)
        {
            LOGM_ERROR(
                TAG,
                "*** %s: Failed to establish the MQTT Client. Please verify your AWS "
                "IoT credentials, "
                "configuration and/or certificate policy. ***",
                DC_FATAL_ERROR);
            LoggerFactory::getLoggerInstance()->shutdown();
            deviceClientAbort("Failed to establish MQTT connection due to credential/configuration error");
            return true;
        }
        else if (SharedCrtResourceManager::SUCCESS == connectionStatus)
        {
            return true;
        }
        else
        {
            return false;
        }
    };
    std::thread attemptConnectionThread(
        [retryConfig, publishLambda] { Retry::exponentialBackoff(retryConfig, publishLambda); });
    attemptConnectionThread.join();
}

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            /**
             * \brief Represents the default set of behavior we expect to exhibit when receiving events from a feature.
             */
            class DefaultClientBaseNotifier final : public ClientBaseNotifier
            {
                void onEvent(Feature *feature, ClientBaseEventNotification notification) override
                {
                    switch (notification)
                    {
                        case ClientBaseEventNotification::FEATURE_STARTED:
                        {
                            LOGM_INFO(
                                TAG, "Client base has been notified that %s has started", feature->getName().c_str());
                            break;
                        }
                        case ClientBaseEventNotification::FEATURE_STOPPED:
                        {
                            LOGM_INFO(TAG, "%s has stopped", feature->getName().c_str());
                            handle_feature_stopped(feature);
                            break;
                        }
                        default:
                        {
                            LOGM_WARN(
                                TAG,
                                "DefaultClientBaseNotifier hit default switch case for feature: %s",
                                feature->getName().c_str());
                        }
                    }
                }

                void onError(Feature *feature, ClientBaseErrorNotification error, const string &msg) override
                {
                    switch (error)
                    {
                        case ClientBaseErrorNotification::SUBSCRIPTION_FAILED:
                        {
                            LOGM_ERROR(TAG, "Subscription rejected: %s", msg.c_str());
                            break;
                        }
                        case ClientBaseErrorNotification::MESSAGE_RECEIVED_AFTER_SHUTDOWN:
                        {
                            LOGM_WARN(TAG, "Received message after feature shutdown: %s", msg.c_str());
                            return;
                        }
                        default:
                        {
                            LOGM_ERROR(
                                TAG,
                                "DefaultClientBaseNotifier hit default ERROR switch case for feature: ",
                                feature->getName().c_str());
                        }
                    }
#ifdef NDEBUG
                        // DC in release mode - we should decide how we want to behave in this scenario
#else
                    // DC in debug mode
                    LOGM_ERROR(
                        TAG,
                        "*** %s: Aborting program due to unrecoverable feature error! ***",
                        DeviceClient::DC_FATAL_ERROR);
                    LoggerFactory::getLoggerInstance()->shutdown();
                    deviceClientAbort(feature->getName() + " encountered an error");
#endif
                }
            };
        } // namespace DeviceClient
    }     // namespace Iot
} // namespace Aws

int main(int argc, char *argv[])
{
    CliArgs cliArgs;
    if (!Config::ParseCliArgs(argc, argv, cliArgs) || !config.init(cliArgs))
    {
        LoggerFactory::getLoggerInstance()->shutdown();
        return 0;
    }

    if (!LoggerFactory::reconfigure(config.config) &&
        dynamic_cast<StdOutLogger *>(LoggerFactory::getLoggerInstance().get()) == nullptr)
    {
        // We attempted to start a non-stdout logger and failed, so we should fall back to STDOUT
        config.config.logConfig.deviceClientLogtype = config.config.logConfig.LOG_TYPE_STDOUT;
        LoggerFactory::reconfigure(config.config);
    }

    EnvUtils envUtils;
    if (envUtils.AppendCwdToPath())
    {
        // Failure to append current working directory is not a fatal error,
        // but some features of device client such as standard job action
        // might not work without explicitly setting path to handler in job document.
        LOG_WARN(TAG, "Unable to append current working directory to PATH environment variable.");
    }

    /**
     * init() is currently responsible for making sure only 1 instance of Device Client is running at a given time.
     * In the future, we may want to move other Device Client startup logic into this function.
     * returns false if an exception is thrown
     */
    if (!init(argc, argv))
    {
        return -1;
    }

    LOGM_INFO(TAG, "Now running AWS IoT Device Client version %s", DEVICE_CLIENT_VERSION_FULL);

    // Register for listening to interrupt signals
    sigset_t sigset;
    memset(&sigset, 0, sizeof(sigset_t));
    int received_signal;
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGHUP);
    sigprocmask(SIG_BLOCK, &sigset, 0);

    shared_ptr<DefaultClientBaseNotifier> listener =
        shared_ptr<DefaultClientBaseNotifier>(new DefaultClientBaseNotifier);
    resourceManager = shared_ptr<SharedCrtResourceManager>(new SharedCrtResourceManager);
    if (!resourceManager.get()->initialize(config.config, &features))
    {
        LOGM_ERROR(TAG, "*** %s: Failed to initialize AWS CRT SDK.", DC_FATAL_ERROR);
        LoggerFactory::getLoggerInstance()->shutdown();
        deviceClientAbort("Failed to initialize AWS CRT SDK");
    }

#if !defined(EXCLUDE_FP)
    if (config.config.fleetProvisioning.enabled &&
        !config.config.fleetProvisioningRuntimeConfig.completedFleetProvisioning)
    {

        /*
         * Establish MQTT connection using claim certificates and private key to provision the device/thing.
         */
#    if !defined(DISABLE_MQTT)
        attemptConnection();
#    endif

        /*
         * Provision Device, parse new runtime conf file and validate its content.
         */
        FleetProvisioning fleetProvisioning;
        if (!fleetProvisioning.ProvisionDevice(resourceManager, config.config) ||
            !config.ParseConfigFile(Config::DEFAULT_FLEET_PROVISIONING_RUNTIME_CONFIG_FILE, true) ||
            !config.ValidateAndStoreRuntimeConfig())
        {
            LOGM_ERROR(
                TAG,
                "*** %s: Failed to Provision thing or validate newly created resources. "
                "Please verify your AWS IoT credentials, "
                "configuration, Fleet Provisioning Template, claim certificate and policy used. ***",
                DC_FATAL_ERROR);
            LoggerFactory::getLoggerInstance()->shutdown();
            deviceClientAbort("Fleet provisioning failed");
        }
        resourceManager->disconnect();
    }
#endif
    /*
     * Establish MQTT connection using permanent certificate and private key to start and run AWS IoT Device Client
     * features.
     */
#if !defined(DISABLE_MQTT)
    attemptConnection();
#endif

#if defined(EXCLUDE_SECURE_ELEMENT)
    if (config.config.secureElement.enabled)
    {
        LOGM_ERROR(
            TAG,
            "*** %s: Secure Element configuration is enabled but feature is not compiled into binary.",
            DC_FATAL_ERROR);
        LoggerFactory::getLoggerInstance()->shutdown();
        deviceClientAbort("Invalid configuration");
    }
    else
    {
        LOG_INFO(TAG, "Provisioning with Secure Elements is disabled");
    }
#endif

#if !defined(EXCLUDE_SHADOW)
#    if !defined(EXCLUDE_CONFIG_SHADOW)
    if (config.config.configShadow.enabled)
    {
        LOG_INFO(TAG, "Config shadow is enabled");
        ConfigShadow configShadow;
        configShadow.reconfigureWithConfigShadow(resourceManager, config.config);
        resourceManager->disconnect();
        attemptConnection();
    }
    else
    {
        LOG_INFO(TAG, "Config shadow is disabled");
    }
#    endif
#endif

    featuresReadWriteLock.lock(); // LOCK

#if !defined(EXCLUDE_JOBS)
    unique_ptr<JobsFeature> jobs;
    if (config.config.jobs.enabled)
    {
        LOG_INFO(TAG, "Jobs is enabled");
        jobs = unique_ptr<JobsFeature>(new JobsFeature());
        jobs->init(resourceManager->getConnection(), listener, config.config);
        features.push_back(jobs.get());
    }
    else
    {
        LOG_INFO(TAG, "Jobs is disabled");
    }
#endif

#if !defined(EXCLUDE_ST)
    unique_ptr<SecureTunnelingFeature> tunneling;
    if (config.config.tunneling.enabled)
    {
        LOG_INFO(TAG, "Secure Tunneling is enabled");
        tunneling = unique_ptr<SecureTunnelingFeature>(new SecureTunnelingFeature());
        tunneling->init(resourceManager, listener, config.config);
        features.push_back(tunneling.get());
    }
    else
    {
        LOG_INFO(TAG, "Secure Tunneling is disabled");
    }
#endif

#if !defined(EXCLUDE_DD)
    unique_ptr<DeviceDefenderFeature> deviceDefender;
    if (config.config.deviceDefender.enabled)
    {
        LOG_INFO(TAG, "Device Defender is enabled");
        deviceDefender = unique_ptr<DeviceDefenderFeature>(new DeviceDefenderFeature());
        deviceDefender->init(resourceManager, listener, config.config);
        features.push_back(deviceDefender.get());
    }
    else
    {
        LOG_INFO(TAG, "Device Defender is disabled");
    }
#endif

#if !defined(EXCLUDE_SHADOW)
#    if !defined(EXCLUDE_SAMPLE_SHADOW)
    unique_ptr<SampleShadowFeature> sampleShadow;
    if (config.config.sampleShadow.enabled)
    {
        LOG_INFO(TAG, "Sample shadow is enabled");
        sampleShadow = unique_ptr<SampleShadowFeature>(new SampleShadowFeature());
        sampleShadow->init(resourceManager, listener, config.config);
        features.push_back(sampleShadow.get());
    }
    else
    {
        LOG_INFO(TAG, "Sample shadow is disabled");
    }
#    endif
#endif

#if !defined(EXCLUDE_SAMPLES)
#    if !defined(EXCLUDE_PUBSUB)
    unique_ptr<PubSubFeature> pubSub;
    if (config.config.pubSub.enabled)
    {
        LOG_INFO(TAG, "PubSub is enabled");
        pubSub = unique_ptr<PubSubFeature>(new PubSubFeature());
        pubSub->init(resourceManager, listener, config.config);
        features.push_back(pubSub.get());
    }
    else
    {
        LOG_INFO(TAG, "Pub Sub is disabled");
    }
#    endif
#endif

#if !defined(EXCLUDE_SENSOR_PUBLISH)
    unique_ptr<SensorPublishFeature> sensorPublish;
    if (config.config.sensorPublish.enabled)
    {
        LOG_INFO(TAG, "Sensor Publish is enabled");
        sensorPublish = unique_ptr<SensorPublishFeature>(new SensorPublishFeature());
        sensorPublish->init(resourceManager, listener, config.config);
        features.push_back(sensorPublish.get());
    }
    else
    {
        LOG_INFO(TAG, "Sensor Publish is disabled");
    }
#else
    if (config.config.sensorPublish.enabled)
    {
        LOGM_ERROR(
            TAG,
            "*** %s: Sensor Publish configuration is enabled but feature is not compiled into binary.",
            DC_FATAL_ERROR);
        LoggerFactory::getLoggerInstance()->shutdown();
        deviceClientAbort("Invalid configuration");
    }
#endif

    resourceManager->startDeviceClientFeatures();
    featuresReadWriteLock.unlock(); // UNLOCK

    // Now allow this thread to sleep until it's interrupted by a signal
    while (true)
    {
        sigwait(&sigset, &received_signal);
        LOGM_INFO(TAG, "Received signal: (%d)", received_signal);
        switch (received_signal)
        {
            case SIGINT:
                shutdown();
                break;
            case SIGHUP:
                resourceManager->dumpMemTrace();
                break;
        }
    }
    return 0;
}
