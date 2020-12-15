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
            class FleetProvisioning
            {
              public:
                bool ProvisionDevice(std::shared_ptr<SharedCrtResourceManager> fpConnection, PlainConfig &config);
                std::string getName();

              private:
                static constexpr char TAG[] = "FleetProvisioning.cpp";
                static constexpr int DEFAULT_WAIT_TIME_SECONDS = 10;
                std::promise<bool> keysPublishCompletedPromise;
                std::promise<bool> keysAcceptedCompletedPromise;
                std::promise<bool> keysRejectedCompletedPromise;
                std::promise<bool> keysCreationCompletedPromise;
                std::promise<void> keysCreationFailedPromise;

                std::promise<bool> registerPublishCompletedPromise;
                std::promise<bool> registerAcceptedCompletedPromise;
                std::promise<bool> registerRejectedCompletedPromise;
                std::promise<bool> registerThingCompletedPromise;
                std::promise<void> registerThingFailedPromise;

                Aws::Crt::String certificateOwnershipToken;
                Aws::Crt::String certificateID;
                Aws::Crt::String certPath;
                Aws::Crt::String keyPath;
                Aws::Crt::String thingName;
                Aws::Crt::String templateName;

                bool RegisterThing(Iotidentity::IotIdentityClient identityClient);
                bool CreateCertificateAndKeys(Iotidentity::IotIdentityClient identityClient);
                void ExportRuntimeConfig(
                    const std::string &file,
                    const std::string &certPath,
                    const std::string &keyPath,
                    const std::string &thingName);
            };
        } // namespace DeviceClient
    }     // namespace Iot
} // namespace Aws
