// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_CLIENTBASENOTIFIER_H
#define DEVICE_CLIENT_CLIENTBASENOTIFIER_H

#include "ClientBaseNotification.h"
#include <string>

namespace Aws {
    namespace Iot {
        namespace DeviceClient {
            class Feature;

            /**
             * This class is used to allow features to notify the client base (Main.cpp) of changes or events
             * that have happened and require attention. The interface methods are implemented at the client
             * base level, and are called upon within the feature.
             */
            class ClientBaseNotifier {
                public:
                    virtual void onEvent(Aws::Iot::DeviceClient::Feature * feature, Aws::Iot::DeviceClient::ClientBaseEventNotification notification) = 0;
                    virtual void onError(Aws::Iot::DeviceClient::Feature * feature, Aws::Iot::DeviceClient::ClientBaseErrorNotification notification, std::string message) = 0;
            };
        }
    }
}

#endif //DEVICE_CLIENT_CLIENTBASENOTIFIER_H