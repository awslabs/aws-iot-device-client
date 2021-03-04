// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_CLIENTBASENOTIFICATION_H
#define DEVICE_CLIENT_CLIENTBASENOTIFICATION_H

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            /**
             * \brief Message from feature -> base indicating an event has occurred
             *
             * When a feature experiences a change in state, such as when the feature stops,
             * it will pass an ClientBaseNotification to the client base (main.cpp) to indicate
             * that the event has occurred. For example, if the client base calls stop() on the feature,
             * the feature can pass an ClientBaseNotification to the client base once it has actually shut down.
             */
            enum class ClientBaseEventNotification
            {
                FEATURE_STARTED,
                FEATURE_STOPPED
            };

            /**
             * \brief Message from feature -> base indicating an ERROR event has occurred
             *
             * Since the client base (main.cpp) is responsible for orchestrating the features (start, stop, error
             * recovery etc.), it's necessary for features to report error conditions to the client base so that it can
             * either perform some bookkeeping or attempt to perform some error recovery. The
             * ClientBaseErrorNotification enables the passing of this error information.
             */
            enum class ClientBaseErrorNotification
            {
                MESSAGE_RECEIVED_AFTER_SHUTDOWN,
                SUBSCRIPTION_FAILED
            };
        } // namespace DeviceClient
    }     // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_CLIENTBASENOTIFICATION_H