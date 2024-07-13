// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "FleetProvisioning.h"
#include "../logging/LoggerFactory.h"
#include "../util/FileUtils.h"

#include <aws/iotidentity/CreateCertificateFromCsrRequest.h>
#include <aws/iotidentity/CreateCertificateFromCsrResponse.h>
#include <aws/iotidentity/CreateCertificateFromCsrSubscriptionRequest.h>
#include <aws/iotidentity/CreateKeysAndCertificateRequest.h>
#include <aws/iotidentity/CreateKeysAndCertificateResponse.h>
#include <aws/iotidentity/CreateKeysAndCertificateSubscriptionRequest.h>
#include <aws/iotidentity/ErrorResponse.h>
#include <aws/iotidentity/IotIdentityClient.h>
#include <aws/iotidentity/RegisterThingRequest.h>
#include <aws/iotidentity/RegisterThingResponse.h>
#include <aws/iotidentity/RegisterThingSubscriptionRequest.h>

#include <arpa/inet.h>
#include <chrono>
#include <ifaddrs.h>
#include <iomanip>
#include <net/if.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wordexp.h>

#ifdef _WIN32
#undef FormatMessage
#endif

using namespace std;
using namespace Aws::Crt;
using namespace Aws::Iotidentity;
using namespace Aws::Iot;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::FleetProvisioningNS;
using namespace Aws::Iot::DeviceClient::Logging;
using namespace Aws::Iot::DeviceClient::Util;


constexpr char FleetProvisioning::TAG[];
constexpr int FleetProvisioning::DEFAULT_WAIT_TIME_SECONDS;

FleetProvisioning::FleetProvisioning() : collectSystemInformation(false) {}

bool FleetProvisioning::CreateCertificateAndKey(Iotidentity::IotIdentityClient identityClient)
{
    LOG_INFO(TAG, "Provisioning new device certificate and private key using CreateKeysAndCertificate API");
    auto onKeysAcceptedSubAck = [this](int ioErr) {
        if (ioErr != AWS_OP_SUCCESS)
        {
            LOGM_ERROR(
                TAG,
                "*** %s: Error subscribing to CreateKeysAndCertificate accepted topic: %s. ***",
                DeviceClient::DC_FATAL_ERROR,
                ErrorDebugString(ioErr));
        }
        keysAcceptedCompletedPromise.set_value(ioErr == AWS_OP_SUCCESS);
    };

    auto onKeysRejectedSubAck = [this](int ioErr) {
        if (ioErr != AWS_OP_SUCCESS)
        {
            LOGM_ERROR(
                TAG,
                "*** %s: Error subscribing to CreateKeysAndCertificate rejected topic: %s. ***",
                DeviceClient::DC_FATAL_ERROR,
                ErrorDebugString(ioErr));
        }
        keysRejectedCompletedPromise.set_value(ioErr == AWS_OP_SUCCESS);
    };

    auto onKeysPublishSubAck = [this](int ioErr) {
        if (ioErr != AWS_OP_SUCCESS)
        {
            LOGM_ERROR(
                TAG,
                "*** %s: Error publishing to CreateKeysAndCertificate topic: %s. ***",
                DeviceClient::DC_FATAL_ERROR,
                ErrorDebugString(ioErr));
        }
        keysPublishCompletedPromise.set_value(ioErr == AWS_OP_SUCCESS);
    };

    auto onKeysAccepted = [this](CreateKeysAndCertificateResponse *response, int ioErr) {
        if (ioErr == AWS_OP_SUCCESS)
        {
            LOGM_INFO(TAG, "CreateKeysAndCertificateResponse certificateId: %s.", response->CertificateId->c_str());
            certificateOwnershipToken = *response->CertificateOwnershipToken;
            Aws::Crt::String certificateID = response->CertificateId->c_str();

            ostringstream certPathStream, keyPathStream;
            certPathStream << keyDir << certificateID << "-certificate.pem.crt";
            keyPathStream << keyDir << certificateID << "-private.pem.key";

            certPath = FileUtils::ExtractExpandedPath(certPathStream.str().c_str()).c_str();
            keyPath = FileUtils::ExtractExpandedPath(keyPathStream.str().c_str()).c_str();

            if (FileUtils::StoreValueInFile(response->CertificatePem->c_str(), certPath.c_str()) &&
                FileUtils::StoreValueInFile(response->PrivateKey->c_str(), keyPath.c_str()))
            {
                LOGM_INFO(
                    TAG, "Stored certificate and private key in %s and %s files", certPath.c_str(), keyPath.c_str());

                LOG_INFO(TAG, "Attempting to set permissions for certificate and private key...");
                chmod(certPath.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                chmod(keyPath.c_str(), S_IRUSR | S_IWUSR);

                if (FileUtils::ValidateFilePermissions(certPath.c_str(), Permissions::PUBLIC_CERT) &&
                    FileUtils::ValidateFilePermissions(keyPath.c_str(), Permissions::PRIVATE_KEY))
                {
                    LOG_INFO(TAG, "Successfully set permissions on provisioned public certificate and private key");
                    keysCreationCompletedPromise.set_value(true);
                }
                else
                {
                    keysCreationCompletedPromise.set_value(false);
                }
            }
            else
            {
                LOGM_ERROR(
                    TAG,
                    "Failed to store public certificate and private key in files %s and %s",
                    certPath.c_str(),
                    keyPath.c_str());
                keysCreationCompletedPromise.set_value(false);
            }
        }
        else
        {
            LOGM_ERROR(
                TAG,
                "*** %s: Error on CreateKeysAndCertificate subscription: %s. ***",
                DeviceClient::DC_FATAL_ERROR,
                ErrorDebugString(ioErr));
            keysCreationCompletedPromise.set_value(false);
        }
    };

    auto onKeysRejected = [this](ErrorResponse *error, int ioErr) {
        if (ioErr == AWS_OP_SUCCESS)
        {
            LOGM_ERROR(
                TAG,
                "*** %s: CreateKeysAndCertificate failed with statusCode %d, errorMessage %s and errorCode %s. ***",
                DeviceClient::DC_FATAL_ERROR,
                *error->StatusCode,
                error->ErrorMessage->c_str(),
                error->ErrorCode->c_str());
        }
        else
        {
            LOGM_ERROR(
                TAG, "*** %s: Error on subscription: %s. ***", DeviceClient::DC_FATAL_ERROR, ErrorDebugString(ioErr));
        }
        keysCreationCompletedPromise.set_value(false);
    };

    /*
     * CreateKeysAndCertificate workflow
     */
    LOG_INFO(TAG, "Subscribing to CreateKeysAndCertificate Accepted and Rejected topics");
    CreateKeysAndCertificateSubscriptionRequest keySubscriptionRequest;
    identityClient.SubscribeToCreateKeysAndCertificateAccepted(
        keySubscriptionRequest, AWS_MQTT_QOS_AT_LEAST_ONCE, onKeysAccepted, onKeysAcceptedSubAck);

    identityClient.SubscribeToCreateKeysAndCertificateRejected(
        keySubscriptionRequest, AWS_MQTT_QOS_AT_LEAST_ONCE, onKeysRejected, onKeysRejectedSubAck);

    auto futureValKeysAcceptedCompletedPromise = keysAcceptedCompletedPromise.get_future();
    auto futureValKeysRejectedCompletedPromise = keysRejectedCompletedPromise.get_future();
    if (futureValKeysAcceptedCompletedPromise.wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
            future_status::timeout ||
        futureValKeysRejectedCompletedPromise.wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
            future_status::timeout)
    {
        LOGM_ERROR(
            TAG,
            "*** %s: Subscribing to CreateKeysAndCertificate Accepted and Rejected topics timed out. ***",
            DeviceClient::DC_FATAL_ERROR);
        return false;
    }
    if (!futureValKeysAcceptedCompletedPromise.get() || !futureValKeysRejectedCompletedPromise.get())
    {
        return false;
    }

    LOG_INFO(TAG, "Publishing to CreateKeysAndCertificate topic");
    CreateKeysAndCertificateRequest createKeysAndCertificateRequest;
    identityClient.PublishCreateKeysAndCertificate(
        createKeysAndCertificateRequest, AWS_MQTT_QOS_AT_LEAST_ONCE, onKeysPublishSubAck);

    auto futureValKeysPublishCompletedPromise = keysPublishCompletedPromise.get_future();
    auto futureValKeysCreationCompletedPromise = keysCreationCompletedPromise.get_future();
    if (futureValKeysPublishCompletedPromise.wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
        future_status::timeout)
    {
        LOGM_ERROR(
            TAG, "*** %s: Publishing to CreateKeysAndCertificate topic timed out. ***", DeviceClient::DC_FATAL_ERROR);
        return false;
    }
    if (futureValKeysCreationCompletedPromise.wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
        future_status::timeout)
    {
        LOGM_ERROR(TAG, "*** %s: Create Keys and Certificate request timed out. ***", DeviceClient::DC_FATAL_ERROR);
        return false;
    }

    return futureValKeysPublishCompletedPromise.get() && futureValKeysCreationCompletedPromise.get();
}

bool FleetProvisioning::CreateCertificateUsingCSR(Iotidentity::IotIdentityClient identityClient)
{
    LOG_INFO(TAG, "Provisioning new device certificate using CreateCertificateFromCsr API");
    auto onCsrAcceptedSubAck = [this](int ioErr) {
        if (ioErr != AWS_OP_SUCCESS)
        {
            LOGM_ERROR(
                TAG,
                "*** %s: Error subscribing to CreateCertificateFromCsr accepted topic: %s. ***",
                DeviceClient::DC_FATAL_ERROR,
                ErrorDebugString(ioErr));
        }
        csrAcceptedCompletedPromise.set_value(ioErr == AWS_OP_SUCCESS);
    };

    auto onCsrRejectedSubAck = [this](int ioErr) {
        if (ioErr != AWS_OP_SUCCESS)
        {
            LOGM_ERROR(
                TAG,
                "*** %s: Error subscribing to CreateCertificateFromCsr rejected topic: %s. ***",
                DeviceClient::DC_FATAL_ERROR,
                ErrorDebugString(ioErr));
        }
        csrRejectedCompletedPromise.set_value(ioErr == AWS_OP_SUCCESS);
    };

    auto onCsrPublishSubAck = [this](int ioErr) {
        if (ioErr != AWS_OP_SUCCESS)
        {
            LOGM_ERROR(
                TAG,
                "*** %s: Error publishing to CreateCertificateFromCsr topic: %s. ***",
                DeviceClient::DC_FATAL_ERROR,
                ErrorDebugString(ioErr));
        }
        csrPublishCompletedPromise.set_value(ioErr == AWS_OP_SUCCESS);
    };

    auto onCsrAccepted = [this](CreateCertificateFromCsrResponse *response, int ioErr) {
        if (ioErr == AWS_OP_SUCCESS)
        {
            LOGM_INFO(TAG, "CreateCertificateFromCsrResponse certificateId: %s. ***", response->CertificateId->c_str());
            certificateOwnershipToken = *response->CertificateOwnershipToken;
            Aws::Crt::String certificateID = response->CertificateId->c_str();

            ostringstream certPathStream;
            certPathStream << keyDir << certificateID << "-certificate.pem.crt";
            certPath = FileUtils::ExtractExpandedPath(certPathStream.str().c_str()).c_str();

            if (FileUtils::StoreValueInFile(response->CertificatePem->c_str(), certPath.c_str()))
            {
                LOGM_INFO(TAG, "Stored certificate in %s file", certPath.c_str());

                LOG_INFO(TAG, "Attempting to set permissions for certificate...");
                chmod(certPath.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                if (FileUtils::ValidateFilePermissions(certPath.c_str(), Permissions::PUBLIC_CERT))
                {
                    LOG_INFO(TAG, "Successfully set permissions on provisioned public certificate");
                    csrCreationCompletedPromise.set_value(true);
                }
                else
                {
                    csrCreationCompletedPromise.set_value(false);
                }
            }
            else
            {
                LOGM_ERROR(TAG, "Failed to store public certificate in file %s", certPath.c_str());
                csrCreationCompletedPromise.set_value(false);
            }
        }
        else
        {
            LOGM_ERROR(
                TAG,
                "*** %s: Error on CreateCertificateFromCsr subscription: %s. ***",
                DeviceClient::DC_FATAL_ERROR,
                ErrorDebugString(ioErr));
            csrCreationCompletedPromise.set_value(false);
        }
    };

    auto onCsrRejected = [this](ErrorResponse *error, int ioErr) {
        if (ioErr == AWS_OP_SUCCESS)
        {
            LOGM_ERROR(
                TAG,
                "*** %s: CreateCertificateFromCsr failed with statusCode %d, errorMessage %s and errorCode %s. ***",
                DeviceClient::DC_FATAL_ERROR,
                *error->StatusCode,
                error->ErrorMessage->c_str(),
                error->ErrorCode->c_str());
        }
        else
        {
            LOGM_ERROR(
                TAG,
                "*** %s: Error on subscription to CreateCertificateFromCsr Rejected topic: %s. ***",
                DeviceClient::DC_FATAL_ERROR,
                ErrorDebugString(ioErr));
        }
        csrCreationCompletedPromise.set_value(false);
    };

    /*
     * CreateCertificateFromCSR workflow
     */

    LOG_INFO(TAG, "Subscribing to CreateCertificateFromCsr Accepted and Rejected topics");
    CreateCertificateFromCsrSubscriptionRequest csrSubscriptionRequest;
    identityClient.SubscribeToCreateCertificateFromCsrAccepted(
        csrSubscriptionRequest, AWS_MQTT_QOS_AT_LEAST_ONCE, onCsrAccepted, onCsrAcceptedSubAck);

    identityClient.SubscribeToCreateCertificateFromCsrRejected(
        csrSubscriptionRequest, AWS_MQTT_QOS_AT_LEAST_ONCE, onCsrRejected, onCsrRejectedSubAck);

    auto futureValCsrAcceptedCompletedPromise = csrAcceptedCompletedPromise.get_future();
    auto futureValCsrRejectedCompletedPromise = csrRejectedCompletedPromise.get_future();
    if (futureValCsrAcceptedCompletedPromise.wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
            future_status::timeout ||
        futureValCsrRejectedCompletedPromise.wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
            future_status::timeout)
    {
        LOGM_ERROR(
            TAG,
            "*** %s: Subscribing to CreateCertificateFromCsr Accepted and Rejected topics timed out. ***",
            DeviceClient::DC_FATAL_ERROR);
        return false;
    }
    if (!futureValCsrAcceptedCompletedPromise.get() || !futureValCsrRejectedCompletedPromise.get())
    {
        return false;
    }

    LOG_INFO(TAG, "Publishing to CreateCertificateFromCsr topic");
    CreateCertificateFromCsrRequest createCertificateFromCsrRequest;
    createCertificateFromCsrRequest.CertificateSigningRequest = csrFile.c_str();
    identityClient.PublishCreateCertificateFromCsr(
        createCertificateFromCsrRequest, AWS_MQTT_QOS_AT_LEAST_ONCE, onCsrPublishSubAck);

    auto futureValCsrPublishCompletedPromise = csrPublishCompletedPromise.get_future();
    auto futureValCsrCreationCompletedPromise = csrCreationCompletedPromise.get_future();
    if (futureValCsrPublishCompletedPromise.wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
        future_status::timeout)
    {
        LOGM_ERROR(
            TAG, "*** %s: Publishing to CreateCertificateFromCsr topic timed out. ***", DeviceClient::DC_FATAL_ERROR);
        return false;
    }
    if (futureValCsrCreationCompletedPromise.wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
        future_status::timeout)
    {
        LOGM_ERROR(TAG, "*** %s: CreateCertificateFromCsr request timed out. ***", DeviceClient::DC_FATAL_ERROR);
        return false;
    }

    return futureValCsrPublishCompletedPromise.get() && futureValCsrCreationCompletedPromise.get();
}
bool FleetProvisioning::RegisterThing(Iotidentity::IotIdentityClient identityClient)
{
    auto onRegisterAcceptedSubAck = [this](int ioErr) {
        if (ioErr != AWS_OP_SUCCESS)
        {
            LOGM_ERROR(
                TAG,
                "*** %s: Error subscribing to RegisterThing accepted: %s. ***",
                DeviceClient::DC_FATAL_ERROR,
                ErrorDebugString(ioErr));
        }
        registerAcceptedCompletedPromise.set_value(ioErr == AWS_OP_SUCCESS);
    };

    auto onRegisterRejectedSubAck = [this](int ioErr) {
        if (ioErr != AWS_OP_SUCCESS)
        {
            LOGM_ERROR(
                TAG,
                "*** %s: Error subscribing to RegisterThing rejected: %s. ***",
                DeviceClient::DC_FATAL_ERROR,
                ErrorDebugString(ioErr));
        }
        registerRejectedCompletedPromise.set_value(ioErr == AWS_OP_SUCCESS);
    };

    auto onRegisterPublishSubAck = [this](int ioErr) {
        if (ioErr != AWS_OP_SUCCESS)
        {
            LOGM_ERROR(
                TAG,
                "*** %s: Error publishing to RegisterThing: %s. ***",
                DeviceClient::DC_FATAL_ERROR,
                ErrorDebugString(ioErr));
        }
        registerPublishCompletedPromise.set_value(ioErr == AWS_OP_SUCCESS);
    };

    auto onRegisterAccepted = [this](RegisterThingResponse *response, int ioErr) {
        if (ioErr == AWS_OP_SUCCESS)
        {
            LOGM_INFO(TAG, "RegisterThingResponse ThingName: %s.", response->ThingName->c_str());
            thingName = response->ThingName->c_str();
            deviceConfig = MapToString(response->DeviceConfiguration).c_str();
        }
        else
        {
            LOGM_ERROR(
                TAG, "*** %s: Error on subscription: %s. ***", DeviceClient::DC_FATAL_ERROR, ErrorDebugString(ioErr));
        }
        registerThingCompletedPromise.set_value(ioErr == AWS_OP_SUCCESS);
    };

    auto onRegisterRejected = [this](ErrorResponse *error, int ioErr) {
        if (ioErr == AWS_OP_SUCCESS)
        {
            LOGM_ERROR(
                TAG,
                "*** %s: RegisterThing failed with statusCode %d, errorMessage %s and errorCode %s. ***",
                DeviceClient::DC_FATAL_ERROR,
                *error->StatusCode,
                error->ErrorMessage->c_str(),
                error->ErrorCode->c_str());
        }
        else
        {
            LOGM_ERROR(
                TAG, "*** %s: Error on subscription: %s. ***", DeviceClient::DC_FATAL_ERROR, ErrorDebugString(ioErr));
        }
        registerThingCompletedPromise.set_value(false);
    };

    LOG_INFO(TAG, "Subscribing to RegisterThing Accepted and Rejected topics");
    RegisterThingSubscriptionRequest registerSubscriptionRequest;
    registerSubscriptionRequest.TemplateName = templateName;

    identityClient.SubscribeToRegisterThingAccepted(
        registerSubscriptionRequest, AWS_MQTT_QOS_AT_LEAST_ONCE, onRegisterAccepted, onRegisterAcceptedSubAck);

    identityClient.SubscribeToRegisterThingRejected(
        registerSubscriptionRequest, AWS_MQTT_QOS_AT_LEAST_ONCE, onRegisterRejected, onRegisterRejectedSubAck);

    auto futureValRegisterAcceptedCompletedPromise = registerAcceptedCompletedPromise.get_future();
    auto futureValRegisterRejectedCompletedPromise = registerRejectedCompletedPromise.get_future();
    if (futureValRegisterAcceptedCompletedPromise.wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
            future_status::timeout ||
        futureValRegisterRejectedCompletedPromise.wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
            future_status::timeout)
    {
        LOGM_ERROR(
            TAG,
            "*** %s: Subscribing to RegisterThing Accepted and Rejected topics timed out. ***",
            DeviceClient::DC_FATAL_ERROR);
        return false;
    }
    if (!futureValRegisterAcceptedCompletedPromise.get() || !futureValRegisterRejectedCompletedPromise.get())
    {
        return false;
    }

    if (collectSystemInformation)
    {
        LOG_INFO(TAG, "Collecting system information");
        if (!PopulateSystemInformation())
        {
            LOGM_ERROR(TAG, "*** %s: Failed to collect system information. ***", DeviceClient::DC_FATAL_ERROR);
            return false;
        }
        LOGM_INFO(TAG, "System information: \n\t%s", MapToString(templateParameters).c_str());
    }

    LOG_INFO(TAG, "Publishing to RegisterThing topic");
    RegisterThingRequest registerThingRequest;
    registerThingRequest.TemplateName = templateName;
    registerThingRequest.CertificateOwnershipToken = certificateOwnershipToken;
    registerThingRequest.Parameters = templateParameters;

    identityClient.PublishRegisterThing(registerThingRequest, AWS_MQTT_QOS_AT_LEAST_ONCE, onRegisterPublishSubAck);

    auto futureValRegisterPublishCompletedPromise = registerPublishCompletedPromise.get_future();
    auto futureValRegisterThingCompletedPromise = registerThingCompletedPromise.get_future();
    if (futureValRegisterPublishCompletedPromise.wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
        future_status::timeout)
    {
        LOGM_ERROR(TAG, "*** %s: Publishing to Register Thing topic timed out. ***", DeviceClient::DC_FATAL_ERROR);
        return false;
    }

    if (futureValRegisterThingCompletedPromise.wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
        future_status::timeout)
    {
        LOGM_ERROR(TAG, "*** %s: Register Thing request timed out. ***", DeviceClient::DC_FATAL_ERROR);
        return false;
    }

    return futureValRegisterPublishCompletedPromise.get() && futureValRegisterThingCompletedPromise.get();
}

bool FleetProvisioning::ProvisionDevice(shared_ptr<SharedCrtResourceManager> fpConnection, PlainConfig &config)
{
    try
    {
        LOG_INFO(TAG, "Fleet Provisioning Feature has been started.");
        collectSystemInformation = config.fleetProvisioning.collectSystemInformation;

        bool didSetup = FileUtils::CreateDirectoryWithPermissions(keyDir.c_str(), S_IRWXU) &&
                        FileUtils::CreateDirectoryWithPermissions(
                            Config::getDefaulConfigDir().c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IROTH | S_IXOTH);
        if (!didSetup)
        {
            LOGM_ERROR(
                TAG,
                "*** %s: Failed to access/create directories required for storage of provisioned certificates, cannot "
                "continue ***",
                DeviceClient::DC_FATAL_ERROR);
            return false;
        }

        IotIdentityClient identityClient(fpConnection.get()->getConnection());
        templateName = config.fleetProvisioning.templateName.value().c_str();
        if (!MapParameters(config.fleetProvisioning.templateParameters))
        {
            return false;
        }

        if (config.fleetProvisioning.csrFile.has_value() && !config.fleetProvisioning.csrFile->empty())
        {
            if (!GetCsrFileContent(config.fleetProvisioning.csrFile->c_str()) ||
                !LocateDeviceKey(config.fleetProvisioning.deviceKey->c_str()) ||
                !CreateCertificateUsingCSR(identityClient))
            {
                LOGM_ERROR(
                    TAG,
                    "*** %s: Fleet Provisioning Feature failed to generate a certificate from a certificate signing "
                    "request (CSR) ***",
                    DeviceClient::DC_FATAL_ERROR);
                return false;
            }
            if (!config.secureElement.enabled && config.fleetProvisioning.deviceKey.has_value() &&
                !config.fleetProvisioning.deviceKey->empty())
            {
                keyPath = config.fleetProvisioning.deviceKey->c_str();
            }
            else
            {
                keyPath = "";
            }
        }
        else
        {
            if (config.secureElement.enabled)
            {
                LOGM_ERROR(
                    TAG,
                    "*** %s: When Secure Tunneling feature is enabled, Device Client only provide support for Fleet "
                    "Provisioning using certificate signing request. Please provide valid CSR file path ***",
                    DeviceClient::DC_FATAL_ERROR);
                return false;
            }
            else
            {
                if (!CreateCertificateAndKey(identityClient))
                {
                    LOGM_ERROR(
                        TAG,
                        "*** %s: Fleet Provisioning Feature failed to create a new certificate and private key ***",
                        DeviceClient::DC_FATAL_ERROR);
                    return false;
                }
            }
        }
        if (RegisterThing(identityClient))
        {
            /*
             * Store data in runtime conf file and update @config object.
             */
            if (!ExportRuntimeConfig(
                    Config::getDefaulFleetProvRTConfigFile(),
                    certPath.c_str(),
                    keyPath.c_str(),
                    thingName.c_str(),
                    deviceConfig.c_str()))
            {
                return false;
            }

            LOGM_INFO(TAG, "Successfully provisioned thing: %s", thingName.c_str());
            return true;
        }
        LOGM_ERROR(
            TAG, "*** %s: Fleet Provisioning Feature failed to provision device. ***", DeviceClient::DC_FATAL_ERROR);
        return false;
    }
    catch (const std::exception &e)
    {
        LOGM_ERROR(TAG, "Error while provisioning device using fleet indexing: %s", e.what());

        return false;
    }
}

/*
 * Helper methods
 */

bool FleetProvisioning::LocateDeviceKey(const string &filePath) const
{
    struct stat info;
    bool locatedDeviceKey = true;
    string expandedPath = FileUtils::ExtractExpandedPath(filePath.c_str());
    if (stat(expandedPath.c_str(), &info) != 0)
    {
        LOGM_ERROR(
            TAG, "Failed to find the device private key %s, file does not exist", Sanitize(expandedPath).c_str());
        locatedDeviceKey = false;
    }
    else
    {
        string parentDir = FileUtils::ExtractParentDirectory(expandedPath.c_str());
        if (!FileUtils::ValidateFilePermissions(parentDir, Permissions::KEY_DIR) ||
            !FileUtils::ValidateFilePermissions(expandedPath.c_str(), Permissions::PRIVATE_KEY))
        {
            LOG_ERROR(TAG, "Incorrect permissions on the device private key file and/or parent directory");
            locatedDeviceKey = false;
        }
    }

    return locatedDeviceKey;
}

bool FleetProvisioning::GetCsrFileContent(const string &filePath)
{
    string expandedPath = FileUtils::ExtractExpandedPath(filePath.c_str());

    struct stat info;
    if (stat(expandedPath.c_str(), &info) != 0)
    {
        LOGM_ERROR(
            TAG,
            "*** %s: Unable to open CSR file %s, file does not exist ***",
            DeviceClient::DC_FATAL_ERROR,
            expandedPath.c_str());
        return false;
    }

    size_t incomingFileSize = FileUtils::GetFileSize(filePath);
    if (2000 < incomingFileSize)
    {
        LOGM_ERROR(
            TAG,
            "*** %s: Refusing to open CSR file %s, file size %zu bytes is greater than allowable limit of %zu bytes "
            "***",
            DeviceClient::DC_FATAL_ERROR,
            filePath.c_str(),
            incomingFileSize,
            2000);
        return false;
    }

    string csrFileParentDir = FileUtils::ExtractParentDirectory(expandedPath.c_str());
    if (!FileUtils::ValidateFilePermissions(csrFileParentDir, Permissions::CSR_DIR) ||
        !FileUtils::ValidateFilePermissions(expandedPath.c_str(), Permissions::CSR_FILE))
    {
        return false;
    }

    ifstream setting(expandedPath.c_str());
    if (!setting.is_open())
    {
        LOGM_ERROR(
            TAG, "*** %s: Unable to open CSR file: '%s' ***", DeviceClient::DC_FATAL_ERROR, expandedPath.c_str());
        return false;
    }
    if (setting.peek() == ifstream::traits_type::eof())
    {
        LOGM_ERROR(TAG, "*** %s: Given CSR file is empty ***", DeviceClient::DC_FATAL_ERROR);
        return false;
    }

    std::string fileContent((std::istreambuf_iterator<char>(setting)), std::istreambuf_iterator<char>());
    csrFile = fileContent;

    LOGM_INFO(TAG, "Successfully fetched CSR file '%s' and stored its content.", filePath.c_str());

    setting.close();
    return true;
}

bool FleetProvisioning::ExportRuntimeConfig(
    const std::string &file,
    const std::string &runtimeCertPath,
    const std::string &runtimeKeyPath,
    const std::string &runtimeThingName,
    const std::string &runtimeDeviceConfig) const
{
    string jsonTemplate = R"({
"%s": {
    "%s": true,
    "%s": "%s",
    "%s": "%s",
    "%s": "%s",
    "%s": {
        %s
        }
    }
})";
    string expandedPath = FileUtils::ExtractExpandedPath(file.c_str());
    ofstream clientConfig(expandedPath);
    if (!clientConfig.is_open())
    {
        LOGM_ERROR(TAG, "*** %s: Unable to open file: '%s' ***", DeviceClient::DC_FATAL_ERROR, file.c_str());
        return false;
    }

    string formattedMsg = FormatMessage(
        jsonTemplate.c_str(),
        PlainConfig::JSON_KEY_RUNTIME_CONFIG,
        PlainConfig::FleetProvisioningRuntimeConfig::JSON_KEY_COMPLETED_FLEET_PROVISIONING,
        PlainConfig::FleetProvisioningRuntimeConfig::JSON_KEY_CERT,
        runtimeCertPath.c_str(),
        PlainConfig::FleetProvisioningRuntimeConfig::JSON_KEY_KEY,
        runtimeKeyPath.c_str(),
        PlainConfig::FleetProvisioningRuntimeConfig::JSON_KEY_THING_NAME,
        runtimeThingName.c_str(),
        PlainConfig::FleetProvisioningRuntimeConfig::JSON_KEY_DEVICE_CONFIG,
        runtimeDeviceConfig.c_str());

    LOG_DEBUG(TAG, formattedMsg.c_str());
    clientConfig << formattedMsg;
    LOGM_INFO(TAG, "Exported runtime configurations to: %s", file.c_str());

    chmod(expandedPath.c_str(), S_IRUSR | S_IWUSR | S_IRGRP);
    FileUtils::ValidateFilePermissions(expandedPath.c_str(), Permissions::RUNTIME_CONFIG_FILE, false);
    return true;
}

bool FleetProvisioning::MapParameters(Aws::Crt::Optional<std::string> params)
{
    if (params.has_value())
    {
        Aws::Crt::JsonObject jsonObj(params.value().c_str());
        if (!jsonObj.WasParseSuccessful())
        {
            LOGM_ERROR(
                Config::TAG,
                "*** Couldn't parse template parameters JSON. GetErrorMessage returns: %s ***",
                jsonObj.GetErrorMessage().c_str());
            return false;
        }

        Aws::Crt::Map<Aws::Crt::String, Aws::Crt::JsonView> pm = jsonObj.View().GetAllObjects();
        for (const auto &x : pm)
        {
            templateParameters.emplace(x.first, x.second.AsString());
        }
    }
    return true;
}

bool FleetProvisioning::PopulateSystemInformation()
{
    // Step 1: Get MAC and IP address of the device.
    if (!CollectNetworkInformation())
    {
        LOG_ERROR(TAG, "*** %s: Failed to collect network information ***");
        return false;
    }

    // Step 2: Get hash values of related files.
    char exec_path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", exec_path, PATH_MAX);
    if (count == -1)
    {
        LOG_ERROR(TAG, "*** %s: Failed to get executable path ***");
        return false;
    }
    std::string exec_path_str(exec_path, count);

    if (!CalculateFileSHA256Value("IoTDeviceClient", exec_path_str))
    {
        LOG_ERROR(TAG, "*** %s: Failed to calculate IoT device client hash value ***");
        return false;
    }

    // Step 3: Get provisioning certificate IDs.
    if (!ObtainCertificateSerialID(certPath.c_str()))
    {
        LOG_ERROR(TAG, "*** %s: Failed to obtain provisioning certificate IDs ***");
        return false;
    }

    return true;
}

bool FleetProvisioning::CollectNetworkInformation()
{
#ifndef _WIN32
    struct ifaddrs *ifap = nullptr;
    char ip[INET6_ADDRSTRLEN];

    if (getifaddrs(&ifap) == -1)
    {
        LOG_ERROR(TAG, "*** %s: Failed to get network interfaces ***");
        return false;
    }

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1)
    {
        freeifaddrs(ifap);

        LOG_ERROR(TAG, "*** %s: Failed to create socket ***");
        return false;
    }

    for (struct ifaddrs *ifa = ifap; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == nullptr)
            continue;

        int family = ifa->ifa_addr->sa_family;
        char *name = ifa->ifa_name;

        // We only search for addresses on eth0 interface.
        if (family == AF_INET && strncmp(name, "eth0", 3) == 0)
        {
            struct in_addr addr = (reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr))->sin_addr;
            inet_ntop(AF_INET, &addr, ip, INET_ADDRSTRLEN);

            struct ifreq ifr;
            unsigned char *mac;

            strncpy(ifr.ifr_name, name, IFNAMSIZ - 1);
            if (ioctl(fd, SIOCGIFHWADDR, &ifr) == -1)
            {
                close(fd);
                freeifaddrs(ifap);

                LOG_ERROR(TAG, "*** %s: Failed to get MAC address for interface ***");
                return false;
            }
            mac = reinterpret_cast<unsigned char *>(ifr.ifr_hwaddr.sa_data);

            Aws::Crt::Optional<std::string> params(FormatMessage(
                R"({"DeviceIPAddress": "%s", "DeviceMACAddress": "%02x:%02x:%02x:%02x:%02x:%02x"})",
                ip,
                mac[0],
                mac[1],
                mac[2],
                mac[3],
                mac[4],
                mac[5]));
            MapParameters(params);
            LOGM_DEBUG(TAG, "Successfully collected network information: %s", params.value().c_str());

            break;
        }
    }

    close(fd);
    freeifaddrs(ifap);
    return true;
#else
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        LOG_ERROR(TAG, "WSAStartup failed");
        return false;
    }

    // Buffer size for GetAdaptersAddresses
    ULONG bufferSize = 15000;
    IP_ADAPTER_ADDRESSES* adapterAddresses = (IP_ADAPTER_ADDRESSES*)malloc(bufferSize);
    if (adapterAddresses == NULL) {
        LOG_ERROR(TAG, "Memory allocation failed");
        WSACleanup();
        return false;
    }

    ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    ULONG result = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, adapterAddresses, &bufferSize);

    if (result == ERROR_BUFFER_OVERFLOW) {
        free(adapterAddresses);
        adapterAddresses = (IP_ADAPTER_ADDRESSES*)malloc(bufferSize);
        if (adapterAddresses == NULL) {
            LOG_ERROR(TAG, "Memory allocation failed");
            WSACleanup();
            return false;
        }
        result = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, adapterAddresses, &bufferSize);
    }

    if (result != NO_ERROR) {
        LOGM_ERROR(TAG, "GetAdaptersAddresses failed with error: %lu", result);
        free(adapterAddresses);
        WSACleanup();
        return false;
    }

    // Arrays to store IP and MAC addresses
    char ip4[INET6_ADDRSTRLEN] = {0};
    unsigned char mac[6] = {0};

    const std::wstring targetInterface = L"Ethernet"; // This is the interface name we want to find

    for (IP_ADAPTER_ADDRESSES* adapter = adapterAddresses; adapter; adapter = adapter->Next) {
        // Compare the adapter name with the target interface name
        if (adapter->FriendlyName != targetInterface) {
            continue;
        }

        // Ignore loopback and adapters without a valid address
        if (adapter->OperStatus != IfOperStatusUp || adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK) {
            continue;
        }

        // Get MAC address
        if (adapter->PhysicalAddressLength != 0) {
            memcpy(mac, adapter->PhysicalAddress, adapter->PhysicalAddressLength);
        }

        // Iterate over all the unicast addresses
        for (IP_ADAPTER_UNICAST_ADDRESS* unicast = adapter->FirstUnicastAddress; unicast; unicast = unicast->Next) {
            // Get the IP address
            int family = unicast->Address.lpSockaddr->sa_family;
            void* address = nullptr;

            if (family == AF_INET) { // IPv4
                address = &((sockaddr_in*)unicast->Address.lpSockaddr)->sin_addr;
                inet_ntop(family, address, ip4, INET6_ADDRSTRLEN);

                Aws::Crt::Optional<std::string> params(FormatMessage(
                    R"({"DeviceIPAddress": "%s", "DeviceMACAddress": "%02x:%02x:%02x:%02x:%02x:%02x"})",
                    ip4,
                    mac[0],
                    mac[1],
                    mac[2],
                    mac[3],
                    mac[4],
                    mac[5]));
                MapParameters(params);
                LOGM_DEBUG(TAG, "Successfully collected network information: %s", params.value().c_str());
            } 
        }
    }

    free(adapterAddresses);
    WSACleanup();
    return true;
#endif
}

bool FleetProvisioning::CalculateFileSHA256Value(const char *fileName, const std::string &filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open())
    {
        LOG_ERROR(TAG, "*** %s: Failed to open file");
        return false;
    }

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx)
    {
        LOG_ERROR(TAG, "*** %s: Failed to create EVP_MD_CTX");
        return false;
    }

    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) != 1)
    {
        LOG_ERROR(TAG, "*** %s: Failed to initialize EVP_DigestInit_ex");
        return false;
    }

    const int bufferSize = 8192;
    char buffer[bufferSize];
    while (file.good())
    {
        file.read(buffer, bufferSize);

        if (!EVP_DigestUpdate(mdctx, buffer, file.gcount()))
        {
            LOG_ERROR(TAG, "*** %s: Failed to update EVP_DigestUpdate");
            return false;
        }
    }

    unsigned char hashBuffer[SHA256_DIGEST_LENGTH];
    if (EVP_DigestFinal_ex(mdctx, hashBuffer, NULL) != 1)
    {
        LOG_ERROR(TAG, "*** %s: Failed to finalize EVP_DigestFinal_ex");
        return false;
    }
    EVP_MD_CTX_free(mdctx);

    std::stringstream ss;
    for (unsigned int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hashBuffer[i];
    }
    std::string hash = ss.str();

    Aws::Crt::Optional<std::string> params(FormatMessage(R"({"%s-SHA256Hash": "%s"})", fileName, hash.c_str()));
    MapParameters(params);
    LOGM_DEBUG(TAG, "File '%s' SHA256 hash: %s", fileName, hash.c_str());

    return true;
}

bool FleetProvisioning::ObtainCertificateSerialID(const char *path)
{
    BIO *certBIO = BIO_new(BIO_s_file());
    if (BIO_read_filename(certBIO, path) <= 0)
    {
        BIO_free(certBIO);

        LOG_ERROR(TAG, "*** %s: Failed to open certificate file ***");
        return false;
    }

    X509 *cert = PEM_read_bio_X509(certBIO, nullptr, nullptr, nullptr);
    if (cert == nullptr)
    {
        BIO_free(certBIO);

        LOG_ERROR(TAG, "*** %s: Failed to load certificate ***");
        return false;
    }

    // Convert ASN1_INTEGER to a readable string
    ASN1_INTEGER *serial = X509_get_serialNumber(cert);
    BIGNUM *bn = ASN1_INTEGER_to_BN(serial, nullptr);
    char *hex = BN_bn2hex(bn);
    std::string serialNumber(hex);

    // Clean up
    BN_free(bn);
    OPENSSL_free(hex);
    X509_free(cert);
    BIO_free(certBIO);

    Aws::Crt::Optional<std::string> params(
        FormatMessage(R"({"ProvisioningCertSerialNumber": "%s"})", serialNumber.c_str()));
    MapParameters(params);
    LOGM_DEBUG(TAG, "Provisioning certificate serial number: %s", serialNumber.c_str());

    return true;
}

