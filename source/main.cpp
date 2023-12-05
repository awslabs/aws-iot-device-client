// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "ClientBaseNotification.h"
#include "Feature.h"
#include "FeatureRegistry.h"
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

constexpr char TAG[] = "Main.cpp";

shared_ptr<FeatureRegistry> features;
shared_ptr<SharedCrtResourceManager> resourceManager;
unique_ptr<LockFile> lockFile;
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
            string thing;
            if (config.config.thingName.has_value() && !config.config.thingName.value().empty())
            {
                thing = config.config.thingName.value();
            }

            lockFile = unique_ptr<LockFile>(new LockFile{filename, argv[0], thing});
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
    LOG_DEBUG(TAG, "Inside of shutdown()");
    if (!attemptingShutdown && features->getSize() != 0)
    {
        attemptingShutdown = true;

        LOG_DEBUG(TAG, "Calling stop all");
        features->stopAll();
    }

    LOG_INFO(TAG, "All features have stopped");
// terminate program
#if !defined(DISABLE_MQTT)
    if (resourceManager != NULL)
    {
        resourceManager->dumpMemTrace();
        resourceManager->disconnect();
        resourceManager.reset();
    }
#endif
    LoggerFactory::getLoggerInstance()->shutdown();
    exit(EXIT_SUCCESS);
}

/**
 * \brief This function shuts down device client when aborting execution due to some type of configuration issue
 *
 * @param reason the reason why the abort is happening
 */
void deviceClientAbort(const string &reason, int exitCode)
{
    if (resourceManager != NULL)
    {
        resourceManager->disconnect();
        resourceManager.reset();
    }
    LoggerFactory::getLoggerInstance()->shutdown();
    cout << "AWS IoT Device Client must abort execution, reason: " << reason << endl;
    cout << "Please check the AWS IoT Device Client logs for more information" << endl;
    exit(exitCode);
}

void attemptConnection()
{
    try
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
                deviceClientAbort(
                    "Failed to establish MQTT connection due to credential/configuration error", EXIT_FAILURE);
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
    catch (const std::exception &e)
    {
        LOGM_ERROR(TAG, "Error attempting to connect: %s", e.what());
        deviceClientAbort("Failure from attemptConnection", EXIT_FAILURE);
    }
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
                    deviceClientAbort(feature->getName() + " encountered an error", EXIT_FAILURE);
#endif
                }
            };
        } // namespace DeviceClient
    }     // namespace Iot
} // namespace Aws

int main(int argc, char *argv[])
{

    if (Config::CheckTerminalArgs(argc, argv))
    {
        LoggerFactory::getLoggerInstance()->shutdown();
        return 0;
    }

    resourceManager = std::make_shared<SharedCrtResourceManager>();
    resourceManager->initializeAllocator();

    CliArgs cliArgs;
    if (!Config::ParseCliArgs(argc, argv, cliArgs) || !config.init(cliArgs))
    {
        LOGM_ERROR(
            TAG,
            "*** %s: AWS IoT Device Client must abort execution, reason: Invalid configuration ***",
            DC_FATAL_ERROR);
        deviceClientAbort("Invalid configuration", EXIT_FAILURE);
    }

    if (!LoggerFactory::reconfigure(config.config) &&
        dynamic_cast<StdOutLogger *>(LoggerFactory::getLoggerInstance().get()) == nullptr)
    {
        // We attempted to start a non-stdout logger and failed, so we should fall back to STDOUT
        config.config.logConfig.deviceClientLogtype = PlainConfig::LogConfig::LOG_TYPE_STDOUT;
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

#if !defined(DISABLE_MQTT)
    /**
     * init() is currently responsible for making sure only 1 instance of Device Client is running at a given time.
     * In the future, we may want to move other Device Client startup logic into this function.
     * returns false if an exception is thrown
     */
    if (!init(argc, argv))
    {
        LOGM_ERROR(TAG, "*** %s: An instance of Device Client is already running.", DC_FATAL_ERROR);
        deviceClientAbort("An instance of Device Client is already running.", EXIT_FAILURE);
    }
#endif

    features = make_shared<FeatureRegistry>();

    LOGM_INFO(TAG, "Now running AWS IoT Device Client version %s", DEVICE_CLIENT_VERSION_FULL);

    // Register for listening to interrupt signals
    sigset_t sigset;
    memset(&sigset, 0, sizeof(sigset_t));
    int received_signal;
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGHUP);
    sigaddset(&sigset, SIGTERM);
    sigprocmask(SIG_BLOCK, &sigset, nullptr);

    auto listener = std::make_shared<DefaultClientBaseNotifier>();
    if (!resourceManager.get()->initialize(config.config, features))
    {
        LOGM_ERROR(TAG, "*** %s: Failed to initialize AWS CRT SDK.", DC_FATAL_ERROR);
        deviceClientAbort("Failed to initialize AWS CRT SDK", EXIT_FAILURE);
    }

#if !defined(EXCLUDE_FP) && !defined(DISABLE_MQTT)
    if (config.config.fleetProvisioning.enabled &&
        !config.config.fleetProvisioningRuntimeConfig.completedFleetProvisioning)
    {
        /*
         * Establish MQTT connection using claim certificates and private key to provision the device/thing.
         */
        attemptConnection();

        /*
         * Provision Device, parse new runtime conf file and validate its content.
         */
        FleetProvisioning fleetProvisioning;
        if (!fleetProvisioning.ProvisionDevice(resourceManager, config.config) ||
            !config.ParseConfigFile(
                Config::DEFAULT_FLEET_PROVISIONING_RUNTIME_CONFIG_FILE,
                Aws::Iot::DeviceClient::Config::FLEET_PROVISIONING_RUNTIME_CONFIG) ||
            !config.ValidateAndStoreRuntimeConfig())
        {
            LOGM_ERROR(
                TAG,
                "*** %s: Failed to Provision thing or validate newly created resources. "
                "Please verify your AWS IoT credentials, "
                "configuration, Fleet Provisioning Template, claim certificate and policy used. ***",
                DC_FATAL_ERROR);
            deviceClientAbort("Fleet provisioning failed", EXIT_FAILURE);
        }
        resourceManager->disconnect();
    }
#else
    if (config.config.fleetProvisioning.enabled)
    {
        LOGM_ERROR(
            TAG,
            "*** %s: Fleet Provisioning configuration is enabled but feature is not compiled into binary.",
            DC_FATAL_ERROR);
        deviceClientAbort(
            "Invalid configuration. Fleet Provisioning configuration is enabled but feature is not "
            "compiled into binary.",
            EXIT_FAILURE);
    }
#endif

#if !defined(DISABLE_MQTT)
    /*
     * Establish MQTT connection using permanent certificate and private key to start and run AWS IoT Device Client
     * features.
     */
    attemptConnection();
#endif

#if defined(EXCLUDE_SECURE_ELEMENT) && !defined(DISABLE_MQTT)
    if (config.config.secureElement.enabled)
    {
        LOGM_ERROR(
            TAG,
            "*** %s: Secure Element configuration is enabled but feature is not compiled into binary.",
            DC_FATAL_ERROR);
        deviceClientAbort("Invalid configuration", EXIT_FAILURE);
    }
    else
    {
        LOG_INFO(TAG, "Provisioning with Secure Elements is disabled");
    }
#else
    if (config.config.secureElement.enabled)
    {
        LOGM_ERROR(
            TAG,
            "*** %s: Secure Element configuration is enabled but feature is not compiled into binary.",
            DC_FATAL_ERROR);
        deviceClientAbort(
            "Invalid configuration. Secure Element configuration is enabled but feature is not compiled into binary.",
            EXIT_FAILURE);
    }
#endif

#if !defined(EXCLUDE_SHADOW) && !defined(EXCLUDE_CONFIG_SHADOW) && !defined(DISABLE_MQTT)
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
#else
    if (config.config.configShadow.enabled)
    {
        LOGM_ERROR(
            TAG,
            "*** %s: Config Shadow configuration is enabled but feature is not compiled into binary.",
            DC_FATAL_ERROR);
        deviceClientAbort("Invalid configuration", EXIT_FAILURE);
    }
#endif

#if !defined(EXCLUDE_JOBS) && !defined(DISABLE_MQTT)
    if (config.config.jobs.enabled)
    {
        shared_ptr<JobsFeature> jobs;
        LOG_INFO(TAG, "Jobs is enabled");
        jobs = make_shared<JobsFeature>();
        jobs->init(resourceManager->getConnection(), listener, config.config);
        features->add(jobs->getName(), jobs);
    }
    else
    {
        LOG_INFO(TAG, "Jobs is disabled");
        features->add(JobsFeature::NAME, nullptr);
    }
#else
    if (config.config.jobs.enabled)
    {
        LOGM_ERROR(
            TAG, "*** %s: Jobs configuration is enabled but feature is not compiled into binary.", DC_FATAL_ERROR);
        deviceClientAbort(
            "Invalid configuration. Config Shadow configuration is enabled but feature is not compiled into binary.",
            EXIT_FAILURE);
    }
#endif

#if !defined(EXCLUDE_ST)
    if (config.config.tunneling.enabled)
    {
        shared_ptr<SecureTunnelingFeature> tunneling;
        LOG_INFO(TAG, "Secure Tunneling is enabled");
        tunneling = make_shared<SecureTunnelingFeature>();
        tunneling->init(resourceManager, listener, config.config);
        features->add(tunneling->getName(), tunneling);
    }
    else
    {
        LOG_INFO(TAG, "Secure Tunneling is disabled");
        features->add(SecureTunnelingFeature::NAME, nullptr);
    }
#else
    if (config.config.tunneling.enabled)
    {
        LOGM_ERROR(
            TAG,
            "*** %s: Secure Tunneling configuration is enabled but feature is not compiled into binary.",
            DC_FATAL_ERROR);
        deviceClientAbort(
            "Invalid configuration. Secure Tunneling configuration is enabled but feature is not "
            "compiled into binary.",
            EXIT_FAILURE);
    }
#endif

#if !defined(EXCLUDE_DD) && !defined(DISABLE_MQTT)
    if (config.config.deviceDefender.enabled)
    {
        shared_ptr<DeviceDefenderFeature> deviceDefender;
        LOG_INFO(TAG, "Device Defender is enabled");
        deviceDefender = make_shared<DeviceDefenderFeature>();
        deviceDefender->init(resourceManager, listener, config.config);
        features->add(deviceDefender->getName(), deviceDefender);
    }
    else
    {
        LOG_INFO(TAG, "Device Defender is disabled");
        features->add(DeviceDefenderFeature::NAME, nullptr);
    }
#else
    if (config.config.sampleShadow.enabled)
    {
        LOGM_ERROR(
            TAG,
            "*** %s: Device Defender configuration is enabled but feature is not compiled into binary.",
            DC_FATAL_ERROR);
        deviceClientAbort(
            "Invalid configuration. Device Defender configuration is enabled but feature is not compiled into binary.",
            EXIT_FAILURE);
    }
#endif

#if !defined(EXCLUDE_SHADOW) && !defined(EXCLUDE_SAMPLE_SHADOW) && !defined(DISABLE_MQTT)
    if (config.config.sampleShadow.enabled)
    {
        shared_ptr<SampleShadowFeature> sampleShadow;
        LOG_INFO(TAG, "Sample shadow is enabled");
        sampleShadow = make_shared<SampleShadowFeature>();
        sampleShadow->init(resourceManager, listener, config.config);
        features->add(sampleShadow->getName(), sampleShadow);
    }
    else
    {
        LOG_INFO(TAG, "Sample shadow is disabled");
        features->add(SampleShadowFeature::NAME, nullptr);
    }
#else
    if (config.config.sampleShadow.enabled)
    {
        LOGM_ERROR(
            TAG,
            "*** %s: Sample Shadow configuration is enabled but feature is not compiled into binary.",
            DC_FATAL_ERROR);
        deviceClientAbort(
            "Invalid configuration. Sample Shadow configuration is enabled but feature is not compiled into binary.",
            EXIT_FAILURE);
    }
#endif

#if !defined(EXCLUDE_SAMPLES) && !defined(EXCLUDE_PUBSUB) && !defined(DISABLE_MQTT)
    if (config.config.pubSub.enabled)
    {
        shared_ptr<PubSubFeature> pubSub;
        LOG_INFO(TAG, "PubSub is enabled");
        pubSub = make_shared<PubSubFeature>();
        pubSub->init(resourceManager, listener, config.config);
        features->add(pubSub->getName(), pubSub);
    }
    else
    {
        LOG_INFO(TAG, "Pub Sub is disabled");
        features->add(PubSubFeature::NAME, nullptr);
    }
#else
    if (config.config.pubSub.enabled)
    {
        LOGM_ERROR(
            TAG,
            "*** %s: PubSub sample configuration is enabled but feature is not compiled into binary.",
            DC_FATAL_ERROR);
        deviceClientAbort(
            "Invalid configuration. PubSub sample configuration is enabled but feature is not compiled into binary.",
            EXIT_FAILURE);
    }
#endif

#if !defined(EXCLUDE_SENSOR_PUBLISH) && !defined(DISABLE_MQTT)
    if (config.config.sensorPublish.enabled)
    {
        shared_ptr<SensorPublishFeature> sensorPublish;
        LOG_INFO(TAG, "Sensor Publish is enabled");
        sensorPublish = make_shared<SensorPublishFeature>();
        sensorPublish->init(resourceManager, listener, config.config);
        features->add(sensorPublish->getName(), sensorPublish);
    }
    else
    {
        LOG_INFO(TAG, "Sensor Publish is disabled");
        features->add(SensorPublishFeature::NAME, nullptr);
    }
#else
    if (config.config.sensorPublish.enabled)
    {
        LOGM_ERROR(
            TAG,
            "*** %s: Sensor Publish configuration is enabled but feature is not compiled into binary.",
            DC_FATAL_ERROR);
        deviceClientAbort(
            "Invalid configuration. Sensor Publish configuration is enabled but feature is not compiled into binary.",
            EXIT_FAILURE);
    }
#endif

    resourceManager->startDeviceClientFeatures();

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
            case SIGTERM:
                shutdown();
                break;
            case SIGHUP:
                resourceManager->dumpMemTrace();
                break;
            default:
                break;
        }
    }
}
