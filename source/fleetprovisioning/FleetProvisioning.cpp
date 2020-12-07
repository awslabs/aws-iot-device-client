// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "FleetProvisioning.h"
#include "../logging/LoggerFactory.h"

#include <iostream>

#include <aws/iotidentity/CreateKeysAndCertificateRequest.h>
#include <aws/iotidentity/CreateKeysAndCertificateResponse.h>
#include <aws/iotidentity/CreateKeysAndCertificateSubscriptionRequest.h>
#include <aws/iotidentity/ErrorResponse.h>
#include <aws/iotidentity/IotIdentityClient.h>
#include <aws/iotidentity/RegisterThingRequest.h>
#include <aws/iotidentity/RegisterThingResponse.h>
#include <aws/iotidentity/RegisterThingSubscriptionRequest.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <string>
#include <thread>

using namespace std;
using namespace Aws::Crt;
using namespace Aws::Iotidentity;
using namespace Aws::Iot;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::Logging;
using namespace Aws::Iot::DeviceClient::Util;

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace FleetProvisioning
            {

                const char *TAG = "FleetProvisioning.cpp";

                bool ProvisionDevice(shared_ptr<SharedCrtResourceManager> fpConnection, PlainConfig &config)
                {
                    IotIdentityClient identityClient(fpConnection.get()->getConnection());
                    Aws::Crt::String certificateOwnershipToken;
                    Aws::Crt::String certificateID;
                    Aws::Crt::String certPath;
                    Aws::Crt::String keyPath;
                    Aws::Crt::String thingName;
                    Aws::Crt::String templateName;
                    templateName = config.fleetProvisioning->templateName.value().c_str();

                    promise<void> keysPublishCompletedPromise;
                    promise<void> keysAcceptedCompletedPromise;
                    promise<void> keysRejectedCompletedPromise;

                    promise<void> registerPublishCompletedPromise;
                    promise<void> registerAcceptedCompletedPromise;
                    promise<void> registerRejectedCompletedPromise;

                    auto onKeysPublishSubAck = [&](int ioErr) {
                        if (ioErr != AWS_OP_SUCCESS)
                        {
                            LOGM_ERROR(
                                FleetProvisioning::TAG,
                                "*** AWS IOT DEVICE CLIENT FATAL ERROR: Error publishing to CreateKeysAndCertificate: "
                                "%s. ***",
                                ErrorDebugString(ioErr));
                            exit(-1);
                        }

                        keysPublishCompletedPromise.set_value();
                    };

                    auto onKeysAcceptedSubAck = [&](int ioErr) {
                        if (ioErr != AWS_OP_SUCCESS)
                        {
                            LOGM_ERROR(
                                FleetProvisioning::TAG,
                                "*** AWS IOT DEVICE CLIENT FATAL ERROR: Error subscribing to CreateKeysAndCertificate "
                                "accepted: %s. ***",
                                ErrorDebugString(ioErr));
                            exit(-1);
                        }

                        keysAcceptedCompletedPromise.set_value();
                    };

                    auto onKeysRejectedSubAck = [&](int ioErr) {
                        if (ioErr != AWS_OP_SUCCESS)
                        {
                            LOGM_ERROR(
                                FleetProvisioning::TAG,
                                "*** AWS IOT DEVICE CLIENT FATAL ERROR: Error subscribing to CreateKeysAndCertificate "
                                "rejected: %s. ***",
                                ErrorDebugString(ioErr));
                            exit(-1);
                        }
                        keysRejectedCompletedPromise.set_value();
                    };

                    auto onKeysAccepted = [&](CreateKeysAndCertificateResponse *response, int ioErr) {
                        if (ioErr == AWS_OP_SUCCESS)
                        {
                            LOGM_INFO(
                                FleetProvisioning::TAG,
                                "CreateKeysAndCertificateResponse certificateId: %s.",
                                response->CertificateId->c_str());
                            certificateID = response->CertificateId->c_str();
                            certPath = certificateID + ".cert.pem";
                            keyPath = certificateID + ".private.pey";
                            FleetProvisioning::StoreValueInFile(response->CertificatePem->c_str(), certPath.c_str());
                            StoreValueInFile(response->PrivateKey->c_str(), keyPath.c_str());
                            certificateOwnershipToken = *response->CertificateOwnershipToken;
                        }
                        else
                        {
                            LOGM_ERROR(
                                FleetProvisioning::TAG,
                                "*** AWS IOT DEVICE CLIENT FATAL ERROR: Error on subscription: %s. ***",
                                ErrorDebugString(ioErr));
                            exit(-1);
                        }
                    };

                    auto onKeysRejected = [&](ErrorResponse *error, int ioErr) {
                        if (ioErr == AWS_OP_SUCCESS)
                        {
                            LOGM_ERROR(
                                FleetProvisioning::TAG,
                                "*** AWS IOT DEVICE CLIENT FATAL ERROR: CreateKeysAndCertificate failed with "
                                "statusCode %d, errorMessage %s and errorCode %s. ***",
                                *error->StatusCode,
                                error->ErrorMessage->c_str(),
                                error->ErrorCode->c_str());
                            exit(-1);
                        }
                        else
                        {
                            LOGM_ERROR(
                                FleetProvisioning::TAG,
                                "*** AWS IOT DEVICE CLIENT FATAL ERROR: Error on subscription: %s. ***",
                                ErrorDebugString(ioErr));
                            exit(-1);
                        }
                    };

                    auto onRegisterAcceptedSubAck = [&](int ioErr) {
                        if (ioErr != AWS_OP_SUCCESS)
                        {
                            LOGM_ERROR(
                                FleetProvisioning::TAG,
                                "*** AWS IOT DEVICE CLIENT FATAL ERROR: Error subscribing to RegisterThing accepted: "
                                "%s. ***",
                                ErrorDebugString(ioErr));
                            exit(-1);
                        }

                        registerAcceptedCompletedPromise.set_value();
                    };

                    auto onRegisterRejectedSubAck = [&](int ioErr) {
                        if (ioErr != AWS_OP_SUCCESS)
                        {
                            LOGM_ERROR(
                                FleetProvisioning::TAG,
                                "*** AWS IOT DEVICE CLIENT FATAL ERROR: Error subscribing to RegisterThing rejected: "
                                "%s. ***",
                                ErrorDebugString(ioErr));
                            exit(-1);
                        }
                        registerRejectedCompletedPromise.set_value();
                    };

                    auto onRegisterAccepted = [&](RegisterThingResponse *response, int ioErr) {
                        if (ioErr == AWS_OP_SUCCESS)
                        {
                            LOGM_INFO(
                                FleetProvisioning::TAG,
                                "RegisterThingResponse ThingName: %s.",
                                response->ThingName->c_str());
                            thingName = response->ThingName->c_str();
                        }
                        else
                        {
                            LOGM_ERROR(
                                FleetProvisioning::TAG,
                                "*** AWS IOT DEVICE CLIENT FATAL ERROR: Error on subscription: %s. ***",
                                ErrorDebugString(ioErr));
                            exit(-1);
                        }
                    };

                    auto onRegisterRejected = [&](ErrorResponse *error, int ioErr) {
                        if (ioErr == AWS_OP_SUCCESS)
                        {
                            LOGM_DEBUG(
                                FleetProvisioning::TAG,
                                "*** AWS IOT DEVICE CLIENT FATAL ERROR: RegisterThing failed with statusCode %d, "
                                "errorMessage %s and errorCode %s. ***",
                                *error->StatusCode,
                                error->ErrorMessage->c_str(),
                                error->ErrorCode->c_str());
                        }
                        else
                        {
                            LOGM_ERROR(
                                FleetProvisioning::TAG,
                                "*** AWS IOT DEVICE CLIENT FATAL ERROR: Error on subscription: %s. ***",
                                ErrorDebugString(ioErr));
                            exit(-1);
                        }
                    };

                    auto onRegisterPublishSubAck = [&](int ioErr) {
                        if (ioErr != AWS_OP_SUCCESS)
                        {
                            LOGM_ERROR(
                                FleetProvisioning::TAG,
                                "*** AWS IOT DEVICE CLIENT FATAL ERROR: Error publishing to RegisterThing: %s. ***",
                                ErrorDebugString(ioErr));
                            exit(-1);
                        }

                        registerPublishCompletedPromise.set_value();
                    };

                    /*
                     * CreateKeysAndCertificate workflow
                     */
                    LOG_INFO(
                        FleetProvisioning::TAG, "Subscribing to CreateKeysAndCertificate Accepted and Rejected topics");
                    CreateKeysAndCertificateSubscriptionRequest keySubscriptionRequest;
                    identityClient.SubscribeToCreateKeysAndCertificateAccepted(
                        keySubscriptionRequest, AWS_MQTT_QOS_AT_LEAST_ONCE, onKeysAccepted, onKeysAcceptedSubAck);

                    identityClient.SubscribeToCreateKeysAndCertificateRejected(
                        keySubscriptionRequest, AWS_MQTT_QOS_AT_LEAST_ONCE, onKeysRejected, onKeysRejectedSubAck);

                    LOG_INFO(FleetProvisioning::TAG, "Publishing to CreateKeysAndCertificate topic");
                    CreateKeysAndCertificateRequest createKeysAndCertificateRequest;
                    identityClient.PublishCreateKeysAndCertificate(
                        createKeysAndCertificateRequest, AWS_MQTT_QOS_AT_LEAST_ONCE, onKeysPublishSubAck);

                    LOG_INFO(FleetProvisioning::TAG, "Subscribing to RegisterThing Accepted and Rejected topics");
                    RegisterThingSubscriptionRequest registerSubscriptionRequest;
                    registerSubscriptionRequest.TemplateName = templateName;

                    identityClient.SubscribeToRegisterThingAccepted(
                        registerSubscriptionRequest,
                        AWS_MQTT_QOS_AT_LEAST_ONCE,
                        onRegisterAccepted,
                        onRegisterAcceptedSubAck);

                    identityClient.SubscribeToRegisterThingRejected(
                        registerSubscriptionRequest,
                        AWS_MQTT_QOS_AT_LEAST_ONCE,
                        onRegisterRejected,
                        onRegisterRejectedSubAck);

                    Sleep(5);

                    LOG_INFO(FleetProvisioning::TAG, "Publishing to RegisterThing topic");
                    RegisterThingRequest registerThingRequest;
                    registerThingRequest.TemplateName = templateName;

                    registerThingRequest.CertificateOwnershipToken = certificateOwnershipToken;

                    identityClient.PublishRegisterThing(
                        registerThingRequest, AWS_MQTT_QOS_AT_LEAST_ONCE, onRegisterPublishSubAck);
                    Sleep(5);

                    keysPublishCompletedPromise.get_future().wait();
                    keysAcceptedCompletedPromise.get_future().wait();
                    keysRejectedCompletedPromise.get_future().wait();
                    registerPublishCompletedPromise.get_future().wait();
                    registerAcceptedCompletedPromise.get_future().wait();
                    registerRejectedCompletedPromise.get_future().wait();

                    /*
                     * Store data in runtime conf file and update @config object.
                     */
                    ExportRuntimeConfig(
                        Config::DEFAULT_RUNTIME_CONFIG_FILE, certPath.c_str(), keyPath.c_str(), thingName.c_str());

                    LOGM_INFO(FleetProvisioning::TAG, "Successfully provisioned thing: %s", thingName.c_str());
                    return true;
                }
                /*
                 * Helper methods
                 */
                void Sleep(int sleeptime)
                {
                    LOGM_INFO(FleetProvisioning::TAG, "Sleeping for %d seconds", sleeptime);
                    std::this_thread::sleep_until(std::chrono::system_clock::now() + std::chrono::seconds(sleeptime));
                }

                string StoreValueInFile(string value, string fileName)
                {
                    ofstream uaFile;
                    uaFile.open(fileName);
                    uaFile << value;
                    uaFile.close();
                    LOGM_INFO(FleetProvisioning::TAG, "Store value in %s file", fileName.c_str());
                    return fileName;
                }

                void ExportRuntimeConfig(
                    const string &file,
                    const string &certPath,
                    const string &keyPath,
                    const string &thingName)
                {
                    string jsonTemplate = R"({
"%s": {
    "%s": true,
    "%s": "%s",
    "%s": "%s",
    "%s": "%s"
    }
})";
                    ofstream clientConfig;
                    clientConfig.open(file);
                    clientConfig << FormatMessage(
                        jsonTemplate.c_str(),
                        PlainConfig::JSON_KEY_RUNTIME_CONFIG,
                        PlainConfig::RuntimeConfig::JSON_KEY_COMPLETED_FLEET_PROVISIONING,
                        PlainConfig::RuntimeConfig::JSON_KEY_CERT,
                        certPath.c_str(),
                        PlainConfig::RuntimeConfig::JSON_KEY_KEY,
                        keyPath.c_str(),
                        PlainConfig::RuntimeConfig::JSON_KEY_THING_NAME,
                        thingName.c_str());
                    clientConfig.close();
                    LOGM_INFO(TAG, "Exported runtime configurations to: %s", file.c_str());
                }

            } // namespace FleetProvisioning
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws