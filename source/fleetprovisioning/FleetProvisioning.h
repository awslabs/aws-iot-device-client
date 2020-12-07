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
                bool ProvisionDevice(std::shared_ptr<SharedCrtResourceManager> fpConnection, PlainConfig &config);
                void Sleep(int sleeptime);
                std::string StoreValueInFile(std::string value, std::string fileName);
                void ExportRuntimeConfig(
                    const std::string &file,
                    const std::string &certPath,
                    const std::string &keyPath,
                    const std::string &thingName);
            } // namespace FleetProvisioning
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws
