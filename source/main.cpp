// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "ClientBaseNotification.h"
#include "Feature.h"
#include "SharedCrtResourceManager.h"
#include "Version.h"
#include "config/Config.h"
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
#include <csignal>
#include <memory>
#include <thread>
#include <vector>

using namespace std;
using namespace Aws::Iot::DeviceClient;
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

const char *TAG = "Main.cpp";

vector<Feature *> features;
mutex featuresReadWriteLock;
bool attemptingShutdown;
Config config;

/**
 * Attempts to perform a graceful shutdown of each running feature. If this function is
 * executed more than once, it will terminate immediately.
 */
void shutdown()
{
    if (!attemptingShutdown)
    {
        attemptingShutdown = true;

        featuresReadWriteLock.lock(); // LOCK
        // Make a copy of the features vector for thread safety
        vector<Feature *> featuresCopy = features;
        featuresReadWriteLock.unlock(); // UNLOCK

        for (auto &feature : featuresCopy)
        {
            LOGM_DEBUG(TAG, "Attempting shutdown of %s", feature->getName().c_str());
            feature->stop();
        }
        LoggerFactory::getLoggerInstance().get()->shutdown();
    }
    else
    {
        // terminate program
        LoggerFactory::getLoggerInstance().get()->shutdown();
        exit(0);
    }
}

void handle_feature_stopped(Feature *feature)
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
                void onEvent(Feature *feature, ClientBaseEventNotification notification)
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

                void onError(Feature *feature, ClientBaseErrorNotification error, string msg)
                {
                    switch (error)
                    {
                        case ClientBaseErrorNotification::SUBSCRIPTION_REJECTED:
                        {
                            LOGM_ERROR(TAG, "Subscription rejected: %s", msg.c_str());
                            break;
                        }
                        case ClientBaseErrorNotification::MESSAGE_RECEIVED_AFTER_SHUTDOWN:
                        {
                            LOGM_ERROR(TAG, "Received message after feature shutdown: %s", msg.c_str());
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
                    LOG_ERROR(
                        TAG,
                        "*** AWS IOT DEVICE CLIENT FATAL ERROR: Aborting program due to unrecoverable feature error! "
                        "***");
                    LoggerFactory::getLoggerInstance()->shutdown();
                    abort();
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
        config.config.logConfig.type = config.config.logConfig.LOG_TYPE_STDOUT;
        LoggerFactory::reconfigure(config.config);
    }

    LOGM_INFO(
        TAG,
        "Now running AWS IoT Device Client version v%d.%d.%d",
        DEVICE_CLIENT_VERSION_MAJOR,
        DEVICE_CLIENT_VERSION_MINOR,
        DEVICE_CLIENT_VERSION_PATCH);

    // Register for listening to interrupt signals
    sigset_t sigset;
    memset(&sigset, 0, sizeof(sigset_t));
    int received_signal;
    sigaddset(&sigset, SIGINT);
    sigprocmask(SIG_BLOCK, &sigset, 0);

    shared_ptr<DefaultClientBaseNotifier> listener =
        shared_ptr<DefaultClientBaseNotifier>(new DefaultClientBaseNotifier);
    shared_ptr<SharedCrtResourceManager> resourceManager =
        shared_ptr<SharedCrtResourceManager>(new SharedCrtResourceManager);
    if (!resourceManager.get()->initialize(config.config))
    {
        LOG_ERROR(
            TAG,
            "*** AWS IOT DEVICE CLIENT FATAL ERROR: Failed to initialize the MQTT Client. Please verify your AWS IoT "
            "credentials and/or "
            "configuration. ***");
        LoggerFactory::getLoggerInstance()->shutdown();
        abort();
    }

#if !defined(EXCLUDE_FP)
    if (config.config.fleetProvisioning.enabled &&
        !config.config.fleetProvisioningRuntimeConfig.completedFleetProvisioning)
    {

        /*
         * Establish MQTT connection using claim certificates and private key to provision device/thing.
         */
#    if !defined(DISABLE_MQTT)
        if (resourceManager.get()->establishConnection(config.config) != SharedCrtResourceManager::SUCCESS)
        {
            LOG_ERROR(
                TAG,
                "*** AWS IOT DEVICE CLIENT FATAL ERROR: Failed to establish the MQTT Client. Please verify your AWS "
                "IoT credentials, "
                "configuration and/or certificate policy. ***");
            LoggerFactory::getLoggerInstance()->shutdown();
            abort();
        }
#    endif

        /*
         * Provision Device, parse new runtime conf file and validate its content.
         */
        FleetProvisioning fleetProvisioning;
        if (!fleetProvisioning.ProvisionDevice(resourceManager, config.config) ||
            !config.ParseConfigFile(Config::DEFAULT_FLEET_PROVISIONING_RUNTIME_CONFIG_FILE) ||
            !config.ValidateAndStoreRuntimeConfig())
        {
            LOG_ERROR(
                TAG,
                "*** AWS IOT DEVICE CLIENT FATAL ERROR: Failed to Provision thing or Validate newly created resources. "
                "Please verify your AWS IoT credentials, "
                "configuration, Fleet Provisioning Template, claim certificate and policy used. ***");
            LoggerFactory::getLoggerInstance()->shutdown();
            abort();
        }
        resourceManager->disconnect();
    }
#endif
    /*
     * Establish MQTT connection using permanent certificate and private key to start and run AWS IoT Device Client
     * features.
     */
#if !defined(DISABLE_MQTT)
    if (resourceManager.get()->establishConnection(config.config) != SharedCrtResourceManager::SUCCESS)
    {
        LOG_ERROR(
            TAG,
            "*** AWS IOT DEVICE CLIENT FATAL ERROR: Failed to initialize the MQTT Client. Please verify your AWS IoT "
            "credentials and/or "
            "configuration. ***");
        LoggerFactory::getLoggerInstance()->shutdown();
        abort();
    }
#endif
    featuresReadWriteLock.lock(); // LOCK

#if !defined(EXCLUDE_JOBS)
    unique_ptr<JobsFeature> jobs;
    if (config.config.jobs.enabled)
    {
        LOG_INFO(TAG, "Jobs is enabled");
        jobs = unique_ptr<JobsFeature>(new JobsFeature());
        jobs->init(resourceManager, listener, config.config);
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

    for (auto &feature : features)
    {
        feature->start();
    }
    featuresReadWriteLock.unlock(); // UNLOCK

    // Now allow this thread to sleep until it's interrupted by a signal
    while (true)
    {
        sigwait(&sigset, &received_signal);
        LOGM_INFO(TAG, "Received signal: (%d)", received_signal);
        if (SIGINT == received_signal)
        {
#if !defined(DISABLE_MQTT)
            resourceManager.get()->disconnect();
#endif
            shutdown();
        }
    }

    return 0;
}