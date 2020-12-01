// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_CLIENTBASENOTIFICATION_H
#define DEVICE_CLIENT_CLIENTBASENOTIFICATION_H

namespace Aws {
    namespace Iot {
        namespace DeviceClient {
            enum class ClientBaseEventNotification {
                FEATURE_STARTED,
                FEATURE_STOPPED
            };

            enum class ClientBaseErrorNotification {
                MESSAGE_RECEIVED_AFTER_SHUTDOWN,
                SUBSCRIPTION_REJECTED
            };
        }
    }
}

#endif //DEVICE_CLIENT_CLIENTBASENOTIFICATION_H