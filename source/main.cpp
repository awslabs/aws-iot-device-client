// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "Feature.h"
#include "jobs/JobsFeature.h"
#include "tunneling/SecureTunnelingFeature.h"
#include "ClientBaseNotification.h"
#include "SharedCrtResourceManager.h"
#include "logging/LoggerFactory.h"
#include "config/Config.h"

#include <csignal>
#include <fstream>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

using namespace std;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::SecureTunneling;

const char * TAG = "Main.cpp";

vector<Feature*> features;
mutex featuresReadWriteLock;
bool attemptingShutdown;
Config config;

/**
 * Attempts to perform a graceful shutdown of each running feature. If this function is
 * executed more than once, it will terminate immediately.
 */
void shutdown() {
    if(!attemptingShutdown) {
        attemptingShutdown = true;

        featuresReadWriteLock.lock();   // LOCK
        // Make a copy of the features vector for thread safety
        vector<Feature*> featuresCopy = features;
        featuresReadWriteLock.unlock(); // UNLOCK

        for(auto & feature : featuresCopy) {
            LOGM_DEBUG(TAG, "Attempting shutdown of %s", feature->get_name().c_str());
            feature->stop();
        }
        LoggerFactory::getLoggerInstance().get()->shutdown();
    } else {
        // terminate program
        LoggerFactory::getLoggerInstance().get()->shutdown();
        exit(0);
    }
}

void handle_feature_stopped(Feature * feature) {
    featuresReadWriteLock.lock(); // LOCK

    for(int i = 0; (unsigned)i < features.size(); i++) {
        if(features.at(i) == feature) {
            // Performing bookkeeping so we know when all features have stopped
            // and the entire program can be shutdown
            features.erase(features.begin() + i);
        }
    }

    const int size = features.size();
    featuresReadWriteLock.unlock(); // UNLOCK

    if(0 == size) {
        LOG_INFO(TAG, "All features have stopped");
        shutdown();
    }
}

void PrintHelpMessage() {
    cout << "\n\n\tAWS IoT Device Client BINARY\n\n";
    cout << "For more documentation, see <Replace with GitHUB repo Link>\n\n";
    cout << "Available sub-commands:\n\n";
    cout << "--help:\t\t\t\t\t\t Get more help on commands\n";
    cout << "--export-default-settings <JSON-File-Location>:\t Export default settings for the AWS IoT Device Client binary to the specified file and exit program\n";
    cout << "--config-file <JSON-File-Location>:\t\t Take settings defined in the specified JSON file and start the binary\n";
    cout << "--enable-jobs:\t\t\t\t\t Enables Jobs feature\n";
    cout << "--enable-tunneling:\t\t\t\t Enables Tunneling feature\n";
    cout << "--endpoint <endpoint-value>:\t\t\t Use Specified Endpoint\n";
    cout << "--cert <Cert-Location>:\t\t\t\t Use Specified Cert file\n";
    cout << "--key <Key-Location>:\t\t\t\t Use Specified Key file\n";
    cout << "--root-ca <Root-CA-Location>:\t\t\t Use Specified Root-CA file" << endl;
    cout << "--thing-name <thing-name-value>:\t\t Use Specified Thing Name" << endl;
}

/**
 * Attempts to export the default settings to the specified JSON file.
 */
void ExportDefaultSetting(string file){
    Aws::Crt::JsonObject root;
    Aws::Crt::JsonObject jobs;
    Aws::Crt::JsonObject tunneling;

    root.WithString(Config::ENDPOINT, "<replace_with_endpoint_value>");
    root.WithString(Config::CERTIFICATE, "<replace_with_certificate_file_location>");
    root.WithString(Config::PRIVATE_KEY, "<replace_with_private_key_file_location>");
    root.WithString(Config::ROOT_CA, "<replace_with_root_ca_file_location>");
    root.WithString(Config::THING_NAME, "<replace_with_thing_name>");
    jobs.WithString(Config::FEATURE_ENABLED, "true");
    tunneling.WithString(Config::FEATURE_ENABLED, "true");
    root.WithObject(Config::JOBS_FEATURE, jobs);
    root.WithObject(Config::TUNNELING_FEATURE, tunneling);

    Aws::Crt::JsonView rootView(root);

    ofstream dcSetting;
    dcSetting.open (file);
    dcSetting << rootView.WriteReadable();
    dcSetting.close();
    LOGM_INFO(TAG, "Exported settings to: %s", file.c_str());
}

/**
 * Logs the error message when command line argument is specified more than once.
 */
void DuplicateArg(string arg){
    LOGM_ERROR(TAG, "*** DC FATAL ERROR: Command Line argument '%s' cannot be specified more than once ***", arg.c_str());
}

/**
 * Attempts to parse and save congifguration/settings provided via commmand line arguments and JSON file into Config object.
 * This Config object will be used further in the program.
 *
 * If any error is observed in 'ParseCli', the program will abort with ERROR message.
 * ParseCli will return 0,1 or 2.
 *      0: Success, continue with program
 *      1: Gracefully shutdown the program
 *      2: ERROR, abort program
 */
int ParseCli(int argc, char ** argv){
    map<string, string> cliArgs;
    for(int i=1; i<argc; i++){
        std::string currentArg = argv[i];
        if(currentArg == "--help"){
            PrintHelpMessage();
            return Config::GRACEFUL_SHUTDOWN;
        } else if(currentArg == "--export-default-settings"){
            if(i+1<argc){
                std::string file = argv[++i];
                ExportDefaultSetting(file);
                LOG_INFO(TAG, "Attempting graceful shutdown of the application after exporting settings");
                return Config::GRACEFUL_SHUTDOWN;
            } else{
                LOGM_ERROR(TAG, "*** DC FATAL ERROR: Command Line argument '%s' was passed without specifying a desired file path for the export ***", currentArg.c_str());
                return Config::ABORT;
            }
        } else if(currentArg == "--config-file"){
            if(!cliArgs.count(Config::CONFIG_FILE)){
                if(i+1<argc){
                    cliArgs[Config::CONFIG_FILE] = argv[++i];
                }else{
                    LOGM_ERROR(TAG, "*** DC FATAL ERROR: Command Line argument '%s' was passed without specifying a desired file path of the JSON config file ***", currentArg.c_str());
                    return Config::ABORT;
                }
            } else{
                DuplicateArg(currentArg);
                return Config::ABORT;
            }
        } else if(currentArg == "--enable-jobs"){
            if(!cliArgs.count(Config::JOBS_FEATURE)){
                cliArgs[Config::JOBS_FEATURE] = "true";
            } else{
                DuplicateArg(currentArg);
                return Config::ABORT;
            }
        } else if(currentArg == "--enable-tunneling"){
            if(!cliArgs.count(Config::TUNNELING_FEATURE)){
                cliArgs[Config::TUNNELING_FEATURE] = "true";
            } else{
                DuplicateArg(currentArg);
                return Config::ABORT;
            }
        } else if(currentArg == "--endpoint") {
            if(!cliArgs.count(Config::ENDPOINT)){
                if(i+1 < argc){
                    cliArgs[Config::ENDPOINT] = argv[++i];
                }else{
                    LOGM_ERROR(TAG, "*** DC FATAL ERROR: Command Line argument '%s' was passed without specifying endpoint value ***", currentArg.c_str());
                    return Config::ABORT;
                }
            } else{
                DuplicateArg(currentArg);
                return Config::ABORT;
            }
        } else if(currentArg == "--cert") {
            if(!cliArgs.count(Config::CERTIFICATE)){
                if(i+1 < argc){
                    cliArgs[Config::CERTIFICATE] = argv[++i];
                }else{
                    LOGM_ERROR(TAG, "*** DC FATAL ERROR: Command Line argument '%s' was passed without specifying a desired file path of the certificate file ***", currentArg.c_str());
                    return Config::ABORT;
                }
            } else{
                DuplicateArg(currentArg);
                return Config::ABORT;
            }
        } else if(currentArg == "--root-ca") {
             if(!cliArgs.count(Config::ROOT_CA)){
                 if(i+1 < argc){
                     cliArgs[Config::ROOT_CA] = argv[++i];
                 }else{
                     LOGM_ERROR(TAG, "*** DC FATAL ERROR: Command Line argument '%s' was passed without specifying a desired file path of the Root-CA file ***", currentArg.c_str());
                     return Config::ABORT;
                 }
             } else{
                 DuplicateArg(currentArg);
                 return Config::ABORT;
             }
        } else if(currentArg == "--key") {
            if(!cliArgs.count(Config::PRIVATE_KEY)){
                if(i+1 < argc){
                    cliArgs[Config::PRIVATE_KEY] = argv[++i];
                }else{
                    LOGM_ERROR(TAG, "*** DC FATAL ERROR: Command Line argument '%s' was passed without specifying a desired file path of the private key file ***", currentArg.c_str());
                    return Config::ABORT;
                }
            } else{
                DuplicateArg(currentArg);
                return Config::ABORT;
            }
        } else if(currentArg == "--thing-name") {
            if(!cliArgs.count(Config::THING_NAME)){
                if(i+1 < argc){
                    cliArgs[Config::THING_NAME] = argv[++i];
                }else{
                    LOGM_ERROR(TAG, "*** DC FATAL ERROR: Command Line argument '%s' was passed without specifying Thing Name value ***", currentArg.c_str());
                    return Config::ABORT;
                }
            } else{
                DuplicateArg(currentArg);
                return Config::ABORT;
            }
        } else{
            LOGM_ERROR(TAG, "*** DC FATAL ERROR: Unrecognised command line argument: %s ***", currentArg.c_str());
            return Config::ABORT;
        }
    }
    return config.init(&cliArgs);
}
/**
 * DefaultClientBaseNotifier represents the default set of behavior we expect
 * to exhibit when receiving events from a feature. We may want to extend this
 * behavior further for particular features or replace it entirely.
 */
class DefaultClientBaseNotifier final : public ClientBaseNotifier {
    void onEvent(Feature* feature, ClientBaseEventNotification notification) {
        switch(notification) {
            case ClientBaseEventNotification::FEATURE_STARTED: {
                LOGM_INFO(TAG, "Client base has been notified that %s has started", feature->get_name().c_str());
                break;
            }
            case ClientBaseEventNotification::FEATURE_STOPPED: {
                LOGM_INFO(TAG, "%s has stopped", feature->get_name().c_str());
                handle_feature_stopped(feature);
                break;
            }
            default: {
                LOGM_WARN(TAG, "DefaultClientBaseNotifier hit default switch case for feature: %s", feature->get_name().c_str());
            }
        }
    }

    void onError(Feature* feature, ClientBaseErrorNotification error, string msg) {
        switch(error) {
            case ClientBaseErrorNotification::SUBSCRIPTION_REJECTED: {
                LOGM_ERROR(TAG, "Subscription rejected: %s", msg.c_str());
                break;
            }
            case ClientBaseErrorNotification::MESSAGE_RECEIVED_AFTER_SHUTDOWN: {
                LOGM_ERROR(TAG, "Received message after feature shutdown: %s", msg.c_str());
            }
            default: {
                LOGM_ERROR(TAG, "DefaultClientBaseNotifier hit default ERROR switch case for feature: ", feature->get_name().c_str());
            }
        }
        #ifdef NDEBUG
            // DC in release mode - we should decide how we want to behave in this scenario
        #else
            // DC in debug mode
            LOG_ERROR(TAG, "*** DC FATAL ERROR: Aborting program due to unrecoverable feature error! ***");
            LoggerFactory::getLoggerInstance()->shutdown();
            abort();
        #endif
    }
};

int main(int argc, char * argv[]) {
    int cliOutput = ParseCli(argc, argv);
    if(cliOutput == Config::SUCCESS){
        LOGM_INFO(TAG, "Starting AWS IoT Device Client with config: %s", config.dcConfig.WriteReadable(true).c_str());

        // Register for listening to interrupt signals
        sigset_t sigset;
        memset(&sigset, 0, sizeof(sigset_t));
        int received_signal;
        sigaddset(&sigset, SIGINT);
        sigprocmask(SIG_BLOCK, &sigset, 0);

        // Initialize features
        shared_ptr<DefaultClientBaseNotifier> listener = shared_ptr<DefaultClientBaseNotifier>(new DefaultClientBaseNotifier);
        shared_ptr<SharedCrtResourceManager> resourceManager = shared_ptr<SharedCrtResourceManager>(new SharedCrtResourceManager);
        if(!resourceManager.get()->initialize(config.dcConfig)) {
            LOG_ERROR(TAG, "*** DC FATAL ERROR: Failed to initialize the MQTT Client. Please verify your AWS IoT credentials and/or configuration. ***");
            LoggerFactory::getLoggerInstance().get()->shutdown();
            abort();
        }

        unique_ptr<JobsFeature> jobs;
        unique_ptr<SecureTunnelingFeature> tunneling;

        featuresReadWriteLock.lock();   // LOCK
        if(config.isFeatureEnabled(Config::JOBS_FEATURE)){
            jobs = unique_ptr<JobsFeature>(new JobsFeature());
            jobs->init(resourceManager, listener, config.dcConfig);
            features.push_back(jobs.get());
        }
        if(config.isFeatureEnabled(Config::TUNNELING_FEATURE)){
            tunneling = unique_ptr<SecureTunnelingFeature>(new SecureTunnelingFeature());
            tunneling->init(resourceManager, listener, config.dcConfig);
            features.push_back(tunneling.get());
        }
        for(auto & feature : features) {
            feature->start();
        }
        featuresReadWriteLock.unlock(); // UNLOCK

        // Now allow this thread to sleep until it's interrupted by a signal
        while(true) {
            sigwait(&sigset, &received_signal);
            LOGM_INFO(TAG, "Received signal: (%d)", received_signal);
            if(SIGINT == received_signal) {
                resourceManager.get()->disconnect();
                shutdown();
            }
        }
    }else if(cliOutput == Config::ABORT){
        LoggerFactory::getLoggerInstance()->shutdown();
        //Error occured while parsing the configuration
        LoggerFactory::getLoggerInstance().get()->shutdown();
        abort();
    }
    return 0;
}
