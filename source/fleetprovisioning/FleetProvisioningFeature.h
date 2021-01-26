// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../SharedCrtResourceManager.h"
#include "../config/Config.h"

#include <aws/iotidentity/IotIdentityClient.h>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace FleetProvisioning
            {
                /**
                 * \brief Provides IoT Fleet Provisioning related functionality within the Device Client
                 */
                class FleetProvisioningFeature
                {
                  public:
                    /**
                     * \brief Provisions device by creating and storing required resources
                     *
                     * @param fpConnection the MqttConnectionManager
                     * @param config configuration information passed in by the user via either the command line or
                     * configuration file
                     * @return returns true if device is provisioned successfully
                     */
                    bool ProvisionDevice(std::shared_ptr<SharedCrtResourceManager> fpConnection, PlainConfig &config);

                  private:
                    /**
                     * \brief Used by the logger to specify that log messages are coming from the Fleet Provisioning
                     * feature
                     */
                    static constexpr char TAG[] = "FleetProvisioningFeature.cpp";
                    /**
                     * \brief The default value in seconds for which Device client will wait for promise variables to be
                     * initialized. These promise variables will be initialized in respective callback methods
                     */
                    static constexpr int DEFAULT_WAIT_TIME_SECONDS = 10;
                    /**
                     * \brief a promise variable to check if publish request for CreateKeysAndCertificate was received
                     */
                    std::promise<bool> keysPublishCompletedPromise;
                    /**
                     * \brief a promise variable to check if subscription request to CreateKeysAndCertificate Accept
                     * topic was executed
                     */
                    std::promise<bool> keysAcceptedCompletedPromise;
                    /**
                     * \brief a promise variable to check if subscription to CreateKeysAndCertificate Reject topic was
                     * executed
                     */
                    std::promise<bool> keysRejectedCompletedPromise;
                    /**
                     * \brief a promise variable to check if publish request for CreateKeysAndCertificate was executed
                     */
                    std::promise<bool> keysCreationCompletedPromise;

                    /**
                     * \brief a promise variable to check if publish request for RegisterThing was received
                     */
                    std::promise<bool> csrPublishCompletedPromise;
                    /**
                     * \brief a promise variable to check if subscription request to CreateCertificateFromCsr Accept
                     * topic was executed
                     */
                    std::promise<bool> csrAcceptedCompletedPromise;
                    /**
                     * \brief a promise variable to check if subscription to CreateCertificateFromCsr Reject topic was
                     * executed
                     */
                    std::promise<bool> csrRejectedCompletedPromise;
                    /**
                     * \brief a promise variable to check if publish request for CreateCertificateFromCsr was executed
                     */
                    std::promise<bool> csrCreationCompletedPromise;

                    /**
                     * \brief a promise variable to check if publish request for RegisterThing was received
                     */
                    std::promise<bool> registerPublishCompletedPromise;
                    /**
                     * \brief a promise variable to check if subscription to RegisterThing Accept topic was
                     * executed
                     */
                    std::promise<bool> registerAcceptedCompletedPromise;
                    /**
                     * \brief a promise variable to check if subscription to RegisterThing Reject topic was
                     * executed
                     */
                    std::promise<bool> registerRejectedCompletedPromise;
                    /**
                     * \brief a promise variable to check if publish request for RegisterThing was executed
                     */
                    std::promise<bool> registerThingCompletedPromise;

                    /**
                     * \brief stores certificate Ownership Token
                     */
                    Aws::Crt::String certificateOwnershipToken;
                    /**
                     * \brief stores certificate file path of newly created certificate
                     */
                    Aws::Crt::String certPath;
                    /**
                     * \brief stores private key file path of newly created private key
                     */
                    Aws::Crt::String keyPath;

                    /**
                     * \brief the location where keys generated by Fleet Provisioning will be stored
                     */
                    std::string keyDir = Config::DEFAULT_KEY_DIR;
                    /**
                     * \brief stores thing name of newly provisioned device
                     */
                    Aws::Crt::String thingName;
                    /**
                     * \brief stores Fleet Provisioning template name
                     */
                    Aws::Crt::String templateName;

                    /**
                     * \brief stores CSR file content
                     */
                    std::string csrFile;

                    /**
                     * \brief creates a new certificate and private key using the AWS certificate authority
                     *
                     * @param identityClient used for subscribing and publishing request for creating resources
                     * @return returns true if resources are created successfully
                     */

                    bool CreateCertificateAndKey(Iotidentity::IotIdentityClient identityClient);

                    /**
                     * \brief generate a certificate from a certificate signing request (CSR) that keeps user private
                     * key secure
                     *
                     * @param identityClient used for subscribing and publishing request for creating resources
                     * @return returns true if resources are created successfully
                     */
                    bool CreateCertificateUsingCSR(Iotidentity::IotIdentityClient identityClient);

                    /**
                     * \brief registers the device with AWS IoT and create cloud resources
                     *
                     * @param identityClient used for subscribing and publishing request for registering and  creating
                     * resources
                     * @return returns true if resources are registered and created successfully
                     */
                    bool RegisterThing(Iotidentity::IotIdentityClient identityClient);

                    /**
                     * \brief exports config of newly created resources to runtime config file
                     *
                     * @param file runtime config file path
                     * @param runtimeCertPath newly created certificate file path
                     * @param runtimeKeyPath newly created private key file path
                     * @param runtimeThingName thing name of newly provisioned device
                     * @return returns true if resources are registered and created successfully
                     */
                    bool ExportRuntimeConfig(
                        const std::string &file,
                        const std::string &runtimeCertPath,
                        const std::string &runtimeKeyPath,
                        const std::string &runtimeThingName);

                    /**
                     * \brief gets CSR file content
                     *
                     * @param filePath CSR file location
                     * @return returns false if client is not able to open the file or if file is empty
                     */
                    bool GetCsrFileContent(const std::string filePath);
                };
            } // namespace FleetProvisioning
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws
