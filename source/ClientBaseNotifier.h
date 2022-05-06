// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_CLIENTBASENOTIFIER_H
#define DEVICE_CLIENT_CLIENTBASENOTIFIER_H

#include "ClientBaseNotification.h"
#include <string>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            class Feature;

            /**
             * \brief Interface the provides event-based messaging between features and the client base (Main.cpp)
             *
             * This class is used to allow features to notify the client base (Main.cpp) of changes or events
             * that have happened and require attention. The interface methods are implemented at the client
             * base level, and are called upon within the feature.
             */
            class ClientBaseNotifier
            {
              public:
                virtual ~ClientBaseNotifier() = default;

                /**
                 * \brief Indicates an event has occurred within a feature
                 *
                 * @param feature a feature should pass reference to itself to aid the agent base in taking action
                 * @param notification the event code representing the event that has occurred
                 */
                virtual void onEvent(
                    Aws::Iot::DeviceClient::Feature *feature,
                    Aws::Iot::DeviceClient::ClientBaseEventNotification notification) = 0;
                /**
                 * \brief Indicates an error event has occurred within a feature
                 *
                 * @param feature a feature should pass reference to itself to aid the client base in taking action
                 * @param notification the error code representing the error that has occurred
                 * @param message an optional message that may be passed regarding the error to provide additional
                 * context
                 */
                virtual void onError(
                    Aws::Iot::DeviceClient::Feature *feature,
                    Aws::Iot::DeviceClient::ClientBaseErrorNotification notification,
                    const std::string &message) = 0;
            };
        } // namespace DeviceClient
    }     // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_CLIENTBASENOTIFIER_H
