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
#include <condition_variable>
#include <string>
#include <sys/stat.h>

using namespace std;
using namespace Aws::Crt;
using namespace Aws::Iotidentity;
using namespace Aws::Iot;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::Logging;
using namespace Aws::Iot::DeviceClient::Util;

constexpr char FleetProvisioning::TAG[];
constexpr int FleetProvisioning::DEFAULT_WAIT_TIME_SECONDS;

string FleetProvisioning::getName()
{
    return "Fleet Provisioning";
}

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
            certPath = certificateID + "-certificate.pem.crt";
            keyPath = certificateID + "-private.pem.key";
            if (FileUtils::StoreValueInFile(response->CertificatePem->c_str(), certPath.c_str()) &&
                FileUtils::StoreValueInFile(response->PrivateKey->c_str(), keyPath.c_str()))
            {
                LOGM_INFO(
                    TAG, "Stored certificate and private key in %s and %s files", certPath.c_str(), keyPath.c_str());
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
        keysCreationFailedPromise.set_value();
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
    if (keysCreationFailedPromise.get_future().wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) !=
        future_status::timeout)
    {
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
            certPath = certificateID + "-certificate.pem.crt";
            if (FileUtils::StoreValueInFile(response->CertificatePem->c_str(), certPath.c_str()))
            {
                LOGM_INFO(TAG, "Stored certificate in %s file", certPath.c_str());
                csrCreationCompletedPromise.set_value(true);
            }
            else
            {
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
        csrCreationFailedPromise.set_value();
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
    createCertificateFromCsrRequest.CertificateSigningRequest = csrFile;
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
    if (csrCreationFailedPromise.get_future().wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) !=
        future_status::timeout)
    {
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
        registerThingFailedPromise.set_value();
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

    if (registerThingFailedPromise.get_future().wait_for(std::chrono::seconds(DEFAULT_WAIT_TIME_SECONDS)) !=
        future_status::timeout)
    {
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

    IotIdentityClient identityClient(fpConnection.get()->getConnection());
    templateName = config.fleetProvisioning.templateName.value().c_str();

    if (config.fleetProvisioning.csrFile.has_value())
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
    ifstream setting(filePath);
    if (!setting.is_open())
    {
        LOGM_ERROR(TAG, "*** AWS IOT DEVICE CLIENT FATAL ERROR: Unable to open CSR file: '%s' ***", filePath.c_str());
        return false;
    }
    if (setting.peek() == ifstream::traits_type::eof())
    {
        LOG_ERROR(TAG, "*** AWS IOT DEVICE CLIENT FATAL ERROR: Given CSR file is empty ***");
        return false;
    }

    Aws::Crt::String fileContent((std::istreambuf_iterator<char>(setting)), std::istreambuf_iterator<char>());
    csrFile = fileContent;

    LOGM_INFO(TAG, "Successfully fetched CSR file '%s' and stored its content.", filePath.c_str());
    setting.close();
    return true;
}

bool FleetProvisioning::ExportRuntimeConfig(
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
    ofstream clientConfig(file);
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
        certPath.c_str(),
        PlainConfig::FleetProvisioningRuntimeConfig::JSON_KEY_KEY,
        keyPath.c_str(),
        PlainConfig::FleetProvisioningRuntimeConfig::JSON_KEY_THING_NAME,
        thingName.c_str());
    clientConfig.close();
    LOGM_INFO(TAG, "Exported runtime configurations to: %s", file.c_str());
    return true;
}