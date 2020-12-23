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

#include <chrono>
#include <string>
#include <sys/stat.h>
#include <wordexp.h>

using namespace std;
using namespace Aws::Crt;
using namespace Aws::Iotidentity;
using namespace Aws::Iot;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::Logging;
using namespace Aws::Iot::DeviceClient::Util;

constexpr char FleetProvisioning::TAG[];
constexpr int FleetProvisioning::DEFAULT_WAIT_TIME_SECONDS;

bool FleetProvisioning::CreateCertificateAndKey(Iotidentity::IotIdentityClient identityClient)
{
    auto onKeysAcceptedSubAck = [&](int ioErr) {
        if (ioErr != AWS_OP_SUCCESS)
        {
            LOGM_ERROR(
                TAG,
                "*** AWS IOT DEVICE CLIENT FATAL ERROR: Error subscribing to CreateKeysAndCertificate "
                "accepted topic: %s. ***",
                ErrorDebugString(ioErr));
        }
        keysAcceptedCompletedPromise.set_value(ioErr == AWS_OP_SUCCESS);
    };

    auto onKeysRejectedSubAck = [&](int ioErr) {
        if (ioErr != AWS_OP_SUCCESS)
        {
            LOGM_ERROR(
                TAG,
                "*** AWS IOT DEVICE CLIENT FATAL ERROR: Error subscribing to CreateKeysAndCertificate "
                "rejected topic: %s. ***",
                ErrorDebugString(ioErr));
        }
        keysRejectedCompletedPromise.set_value(ioErr == AWS_OP_SUCCESS);
    };

    auto onKeysPublishSubAck = [&](int ioErr) {
        if (ioErr != AWS_OP_SUCCESS)
        {
            LOGM_ERROR(
                TAG,
                "*** AWS IOT DEVICE CLIENT FATAL ERROR: Error publishing to CreateKeysAndCertificate topic: "
                "%s. ***",
                ErrorDebugString(ioErr));
        }
        keysPublishCompletedPromise.set_value(ioErr == AWS_OP_SUCCESS);
    };

    auto onKeysAccepted = [&](CreateKeysAndCertificateResponse *response, int ioErr) {
        if (ioErr == AWS_OP_SUCCESS)
        {
            LOGM_INFO(TAG, "CreateKeysAndCertificateResponse certificateId: %s.", response->CertificateId->c_str());
            Aws::Crt::String certificateID = response->CertificateId->c_str();
            certificateOwnershipToken = *response->CertificateOwnershipToken;

            ostringstream certPathStream, keyPathStream;
            wordexp_t expandedCertPath, expandedKeyPath;

            certPathStream << keyDir << certificateID << "-certificate.pem.crt";
            keyPathStream << keyDir << certificateID << "-private.pem.key";

            wordexp(certPathStream.str().c_str(), &expandedCertPath, 0);
            certPath = expandedCertPath.we_wordv[0];
            wordexp(keyPathStream.str().c_str(), &expandedKeyPath, 0);
            keyPath = expandedKeyPath.we_wordv[0];

            wordfree(&expandedCertPath);
            wordfree(&expandedKeyPath);
            if (FileUtils::StoreValueInFile(response->CertificatePem->c_str(), certPath.c_str()) &&
                FileUtils::StoreValueInFile(response->PrivateKey->c_str(), keyPath.c_str()))
            {
                LOGM_INFO(
                    TAG, "Stored certificate and private key in %s and %s files", certPath.c_str(), keyPath.c_str());

                LOG_INFO(TAG, "Attempting to set permissions for certificate and private key...");
                chmod(certPath.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                chmod(keyPath.c_str(), S_IRUSR | S_IWUSR);

                int actualCertPermissions = FileUtils::getFilePermissions(certPath.c_str());
                int actualKeyPermissions = FileUtils::getFilePermissions(keyPath.c_str());
                if (Permissions::PUBLIC_CERT != actualCertPermissions ||
                    Permissions::PRIVATE_KEY != actualKeyPermissions)
                {
                    LOGM_ERROR(
                        TAG,
                        "Failed to set permissions for provisioned cert and/or private key: {cert: {desired: %d, "
                        "actual: %d}, key: {desired: %d, actual: %d}}",
                        Permissions::PUBLIC_CERT,
                        actualCertPermissions,
                        Permissions::PRIVATE_KEY,
                        actualKeyPermissions);
                    keysCreationCompletedPromise.set_value(false);
                }
                else
                {
                    LOG_INFO(TAG, "Successfully set permissions on provisioned public certificate and private key");
                    keysCreationCompletedPromise.set_value(true);
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
                "*** AWS IOT DEVICE CLIENT FATAL ERROR: Error on CreateKeysAndCertificate subscription: %s. ***",
                ErrorDebugString(ioErr));
            keysCreationCompletedPromise.set_value(false);
        }
    };

    auto onKeysRejected = [&](ErrorResponse *error, int ioErr) {
        if (ioErr == AWS_OP_SUCCESS)
        {
            LOGM_ERROR(
                TAG,
                "*** AWS IOT DEVICE CLIENT FATAL ERROR: CreateKeysAndCertificate failed with "
                "statusCode %d, errorMessage %s and errorCode %s. ***",
                *error->StatusCode,
                error->ErrorMessage->c_str(),
                error->ErrorCode->c_str());
        }
        else
        {
            LOGM_ERROR(
                TAG, "*** AWS IOT DEVICE CLIENT FATAL ERROR: Error on subscription: %s. ***", ErrorDebugString(ioErr));
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
        LOG_ERROR(
            TAG,
            "*** AWS IOT DEVICE CLIENT FATAL ERROR: Subscribing to CreateKeysAndCertificate Accepted and Rejected "
            "topics timed out. ***");
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
        LOG_ERROR(
            TAG, "*** AWS IOT DEVICE CLIENT FATAL ERROR: Publishing to CreateKeysAndCertificate topic timed out. ***");
        return false;
    }
    if (futureValKeysCreationCompletedPromise.wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
        future_status::timeout)
    {
        LOG_ERROR(TAG, "*** AWS IOT DEVICE CLIENT FATAL ERROR: Create Keys and Certificate request timed out. ***");
        return false;
    }

    return futureValKeysPublishCompletedPromise.get() && futureValKeysCreationCompletedPromise.get();
}

bool FleetProvisioning::CreateCertificateUsingCSR(Iotidentity::IotIdentityClient identityClient)
{
    auto onCsrAcceptedSubAck = [&](int ioErr) {
        if (ioErr != AWS_OP_SUCCESS)
        {
            LOGM_ERROR(
                TAG,
                "*** AWS IOT DEVICE CLIENT FATAL ERROR: Error subscribing to CreateCertificateFromCsr accepted topic: "
                "%s. "
                "***",
                ErrorDebugString(ioErr));
        }
        csrAcceptedCompletedPromise.set_value(ioErr == AWS_OP_SUCCESS);
    };

    auto onCsrRejectedSubAck = [&](int ioErr) {
        if (ioErr != AWS_OP_SUCCESS)
        {
            LOGM_ERROR(
                TAG,
                "*** AWS IOT DEVICE CLIENT FATAL ERROR: Error subscribing to CreateCertificateFromCsr rejected topic: "
                "%s. "
                "***",
                ErrorDebugString(ioErr));
        }
        csrRejectedCompletedPromise.set_value(ioErr == AWS_OP_SUCCESS);
    };

    auto onCsrPublishSubAck = [&](int ioErr) {
        if (ioErr != AWS_OP_SUCCESS)
        {
            LOGM_ERROR(
                TAG,
                "*** AWS IOT DEVICE CLIENT FATAL ERROR: Error publishing to CreateCertificateFromCsr topic: %s. ***",
                ErrorDebugString(ioErr));
        }
        csrPublishCompletedPromise.set_value(ioErr == AWS_OP_SUCCESS);
    };

    auto onCsrAccepted = [&](CreateCertificateFromCsrResponse *response, int ioErr) {
        if (ioErr == AWS_OP_SUCCESS)
        {
            LOGM_INFO(TAG, "CreateCertificateFromCsrResponse certificateId: %s. ***", response->CertificateId->c_str());
            Aws::Crt::String certificateID = response->CertificateId->c_str();
            certificateOwnershipToken = *response->CertificateOwnershipToken;

            ostringstream certPathStream;
            certPathStream << keyDir << certificateID << "-certificate.pem.crt";
            wordexp_t expandedPath;
            wordexp(certPathStream.str().c_str(), &expandedPath, 0);
            certPath = expandedPath.we_wordv[0];
            wordfree(&expandedPath);

            if (FileUtils::StoreValueInFile(response->CertificatePem->c_str(), certPath.c_str()))
            {
                LOGM_INFO(TAG, "Stored certificate in %s file", certPath.c_str());

                LOG_INFO(TAG, "Attempting to set permissions for certificate...");
                chmod(certPath.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                int actualCertPermissions = FileUtils::getFilePermissions(certPath.c_str());
                if (Permissions::PUBLIC_CERT != actualCertPermissions)
                {
                    LOGM_ERROR(
                        TAG,
                        "Failed to set permissions for provisioned cert: {cert: {desired: %d, "
                        "actual: %d}}",
                        Permissions::PUBLIC_CERT,
                        actualCertPermissions);
                    csrCreationCompletedPromise.set_value(false);
                }
                else
                {
                    LOG_INFO(TAG, "Successfully set permissions on provisioned public certificate");
                    csrCreationCompletedPromise.set_value(true);
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
                "*** AWS IOT DEVICE CLIENT FATAL ERROR: Error on CreateCertificateFromCsr subscription: %s. ***",
                ErrorDebugString(ioErr));
            csrCreationCompletedPromise.set_value(false);
        }
    };

    auto onCsrRejected = [&](ErrorResponse *error, int ioErr) {
        if (ioErr == AWS_OP_SUCCESS)
        {
            LOGM_ERROR(
                TAG,
                "*** AWS IOT DEVICE CLIENT FATAL ERROR: CreateCertificateFromCsr failed with "
                "statusCode %d, errorMessage %s and errorCode %s. ***",
                *error->StatusCode,
                error->ErrorMessage->c_str(),
                error->ErrorCode->c_str());
        }
        else
        {
            LOGM_ERROR(
                TAG,
                "*** AWS IOT DEVICE CLIENT FATAL ERROR: Error on subscription to CreateCertificateFromCsr Rejected "
                "topic: %s. ***",
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
        LOG_ERROR(
            TAG,
            "*** AWS IOT DEVICE CLIENT FATAL ERROR: Subscribing to CreateCertificateFromCsr Accepted and Rejected "
            "topics timed out. ***");
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
        LOG_ERROR(
            TAG, "*** AWS IOT DEVICE CLIENT FATAL ERROR: Publishing to CreateCertificateFromCsr topic timed out. ***");
        return false;
    }
    if (futureValCsrCreationCompletedPromise.wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
        future_status::timeout)
    {
        LOG_ERROR(TAG, "*** AWS IOT DEVICE CLIENT FATAL ERROR: CreateCertificateFromCsr request timed out. ***");
        return false;
    }

    return futureValCsrPublishCompletedPromise.get() && futureValCsrCreationCompletedPromise.get();
}
bool FleetProvisioning::RegisterThing(Iotidentity::IotIdentityClient identityClient)
{
    auto onRegisterAcceptedSubAck = [&](int ioErr) {
        if (ioErr != AWS_OP_SUCCESS)
        {
            LOGM_ERROR(
                TAG,
                "*** AWS IOT DEVICE CLIENT FATAL ERROR: Error subscribing to RegisterThing accepted: "
                "%s. ***",
                ErrorDebugString(ioErr));
        }
        registerAcceptedCompletedPromise.set_value(ioErr == AWS_OP_SUCCESS);
    };

    auto onRegisterRejectedSubAck = [&](int ioErr) {
        if (ioErr != AWS_OP_SUCCESS)
        {
            LOGM_ERROR(
                TAG,
                "*** AWS IOT DEVICE CLIENT FATAL ERROR: Error subscribing to RegisterThing rejected: "
                "%s. ***",
                ErrorDebugString(ioErr));
        }
        registerRejectedCompletedPromise.set_value(ioErr == AWS_OP_SUCCESS);
    };

    auto onRegisterPublishSubAck = [&](int ioErr) {
        if (ioErr != AWS_OP_SUCCESS)
        {
            LOGM_ERROR(
                TAG,
                "*** AWS IOT DEVICE CLIENT FATAL ERROR: Error publishing to RegisterThing: %s. ***",
                ErrorDebugString(ioErr));
        }
        registerPublishCompletedPromise.set_value(ioErr == AWS_OP_SUCCESS);
    };

    auto onRegisterAccepted = [&](RegisterThingResponse *response, int ioErr) {
        if (ioErr == AWS_OP_SUCCESS)
        {
            LOGM_INFO(TAG, "RegisterThingResponse ThingName: %s.", response->ThingName->c_str());
            thingName = response->ThingName->c_str();
        }
        else
        {
            LOGM_ERROR(
                TAG, "*** AWS IOT DEVICE CLIENT FATAL ERROR: Error on subscription: %s. ***", ErrorDebugString(ioErr));
        }
        registerThingCompletedPromise.set_value(ioErr == AWS_OP_SUCCESS);
    };

    auto onRegisterRejected = [&](ErrorResponse *error, int ioErr) {
        if (ioErr == AWS_OP_SUCCESS)
        {
            LOGM_ERROR(
                TAG,
                "*** AWS IOT DEVICE CLIENT FATAL ERROR: RegisterThing failed with statusCode %d, "
                "errorMessage %s and errorCode %s. ***",
                *error->StatusCode,
                error->ErrorMessage->c_str(),
                error->ErrorCode->c_str());
        }
        else
        {
            LOGM_ERROR(
                TAG, "*** AWS IOT DEVICE CLIENT FATAL ERROR: Error on subscription: %s. ***", ErrorDebugString(ioErr));
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
        LOG_ERROR(
            TAG,
            "*** AWS IOT DEVICE CLIENT FATAL ERROR: Subscribing to RegisterThing Accepted and Rejected topics timed "
            "out. ***");
        return false;
    }
    if (!futureValRegisterAcceptedCompletedPromise.get() || !futureValRegisterRejectedCompletedPromise.get())
    {
        return false;
    }

    LOG_INFO(TAG, "Publishing to RegisterThing topic");
    RegisterThingRequest registerThingRequest;
    registerThingRequest.TemplateName = templateName;
    registerThingRequest.CertificateOwnershipToken = certificateOwnershipToken;

    identityClient.PublishRegisterThing(registerThingRequest, AWS_MQTT_QOS_AT_LEAST_ONCE, onRegisterPublishSubAck);

    auto futureValRegisterPublishCompletedPromise = registerPublishCompletedPromise.get_future();
    auto futureValRegisterThingCompletedPromise = registerThingCompletedPromise.get_future();
    if (futureValRegisterPublishCompletedPromise.wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
        future_status::timeout)
    {
        LOG_ERROR(TAG, "*** AWS IOT DEVICE CLIENT FATAL ERROR: Publishing to Register Thing topic timed out. ***");
        return false;
    }

    if (futureValRegisterThingCompletedPromise.wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) ==
        future_status::timeout)
    {
        LOG_ERROR(TAG, "*** AWS IOT DEVICE CLIENT FATAL ERROR: Register Thing request timed out. ***");
        return false;
    }

    return futureValRegisterPublishCompletedPromise.get() && futureValRegisterThingCompletedPromise.get();
}

bool FleetProvisioning::ProvisionDevice(shared_ptr<SharedCrtResourceManager> fpConnection, PlainConfig &config)
{
    // TODO: Add ClientBaseNotifier to log events
    LOG_INFO(TAG, "Fleet Provisioning Feature has been started.");

    bool didSetup = FileUtils::createDirectoryWithPermissions(keyDir.c_str(), S_IRWXU) &&
                    FileUtils::createDirectoryWithPermissions(Config::DEFAULT_CONFIG_DIR, S_IRWXU);
    if (!didSetup)
    {
        LOG_ERROR(
            TAG,
            "Failed to access/create directories required for storage of provisioned certificates, cannot continue");
        return false;
    }

    IotIdentityClient identityClient(fpConnection.get()->getConnection());
    templateName = config.fleetProvisioning.templateName.value().c_str();

    if (config.fleetProvisioning.csrFile.has_value() && !config.fleetProvisioning.csrFile->empty())
    {
        if (!GetCsrFileContent(config.fleetProvisioning.csrFile->c_str()) || !CreateCertificateUsingCSR(identityClient))
        {
            LOG_ERROR(
                TAG,
                "Fleet Provisioning Feature failed to generate a certificate from a certificate signing request (CSR)");
            return false;
        }
        keyPath = config.key->c_str();
    }
    else
    {
        if (!CreateCertificateAndKey(identityClient))
        {
            LOG_ERROR(TAG, "Fleet Provisioning Feature failed to create a new certificate and private key");
            return false;
        }
    }
    if (RegisterThing(identityClient))
    {
        /*
         * Store data in runtime conf file and update @config object.
         */
        if (!ExportRuntimeConfig(
                Config::DEFAULT_FLEET_PROVISIONING_RUNTIME_CONFIG_FILE,
                certPath.c_str(),
                keyPath.c_str(),
                thingName.c_str()))
        {
            return false;
        }

        LOGM_INFO(TAG, "Successfully provisioned thing: %s", thingName.c_str());
        return true;
    }
    LOG_ERROR(TAG, "Fleet Provisioning Feature failed to provision device.");
    return false;
}

/*
 * Helper methods
 */

bool FleetProvisioning::GetCsrFileContent(const string filePath)
{
    wordexp_t word;
    wordexp(filePath.c_str(), &word, 0);
    string expandedPath = word.we_wordv[0];
    wordfree(&word);

    struct stat info;
    if (stat(expandedPath.c_str(), &info) != 0)
    {
        LOGM_ERROR(TAG, "Unable to open CSR file %s, file does not exist", expandedPath.c_str());
        return false;
    }

    size_t incomingFileSize = FileUtils::getFileSize(filePath);
    if (2000 < incomingFileSize)
    {
        LOGM_ERROR(
            TAG,
            "Refusing to open CSR file %s, file size %zu bytes is greater than allowable limit of %zu bytes",
            filePath.c_str(),
            incomingFileSize,
            2000);
        return false;
    }

    string csrFileParentDir = FileUtils::extractParentDirectory(expandedPath.c_str());
    int actualCsrDirPermissions = FileUtils::getFilePermissions(csrFileParentDir);
    int actualCsrFilePermissions = FileUtils::getFilePermissions(expandedPath.c_str());
    if (Permissions::CSR_DIR != actualCsrDirPermissions)
    {
        LOGM_ERROR(
            TAG,
            "File permissions for CSR file directory %s is not set to the recommended setting of %d, found %d "
            "instead",
            csrFileParentDir.c_str(),
            Permissions::CSR_DIR,
            actualCsrDirPermissions);
        return false;
    }
    if (Permissions::CSR_FILE != actualCsrFilePermissions)
    {
        LOGM_ERROR(
            TAG,
            "File permissions for CSR file %s are not set to the recommended setting of %d, found %d instead",
            expandedPath.c_str(),
            Permissions::CSR_FILE,
            actualCsrFilePermissions);
        return false;
    }

    ifstream setting(expandedPath.c_str());
    if (!setting.is_open())
    {
        LOGM_ERROR(
            TAG, "*** AWS IOT DEVICE CLIENT FATAL ERROR: Unable to open CSR file: '%s' ***", expandedPath.c_str());
        return false;
    }
    if (setting.peek() == ifstream::traits_type::eof())
    {
        LOG_ERROR(TAG, "*** AWS IOT DEVICE CLIENT FATAL ERROR: Given CSR file is empty ***");
        return false;
    }

    std::string fileContent((std::istreambuf_iterator<char>(setting)), std::istreambuf_iterator<char>());
    //    TODO: Pending Security review. (Saving CSR file content in Class private variable)
    csrFile = fileContent;

    LOGM_INFO(TAG, "Successfully fetched CSR file '%s' and stored its content.", filePath.c_str());
    setting.close();
    return true;
}

bool FleetProvisioning::ExportRuntimeConfig(
    const string &file,
    const string &runtimeCertPath,
    const string &runtimeKeyPath,
    const string &runtimeThingName)
{
    string jsonTemplate = R"({
"%s": {
    "%s": true,
    "%s": "%s",
    "%s": "%s",
    "%s": "%s"
    }
})";
    wordexp_t word;
    wordexp(file.c_str(), &word, 0);
    string expandedPath = word.we_wordv[0];
    wordfree(&word);
    ofstream clientConfig(expandedPath);
    if (!clientConfig.is_open())
    {
        LOGM_ERROR(TAG, "Unable to open file: '%s'", file.c_str());
        return false;
    }
    clientConfig << FormatMessage(
        jsonTemplate.c_str(),
        PlainConfig::JSON_KEY_RUNTIME_CONFIG,
        PlainConfig::FleetProvisioningRuntimeConfig::JSON_KEY_COMPLETED_FLEET_PROVISIONING,
        PlainConfig::FleetProvisioningRuntimeConfig::JSON_KEY_CERT,
        runtimeCertPath.c_str(),
        PlainConfig::FleetProvisioningRuntimeConfig::JSON_KEY_KEY,
        runtimeKeyPath.c_str(),
        PlainConfig::FleetProvisioningRuntimeConfig::JSON_KEY_THING_NAME,
        runtimeThingName.c_str());
    clientConfig.close();
    LOGM_INFO(TAG, "Exported runtime configurations to: %s", file.c_str());

    chmod(file.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    const int actual = FileUtils::getFilePermissions(file.c_str());
    if (Permissions::RUNTIME_CONFIG_FILE != actual)
    {
        LOGM_WARN(
            TAG,
            "Failed to set appropriate permissions on runtime config %s, desired %d but found %d",
            file.c_str(),
            Permissions::RUNTIME_CONFIG_FILE,
            actual);
    }
    return true;
}