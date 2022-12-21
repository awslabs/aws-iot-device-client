// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_FEATURE_H
#define DEVICE_CLIENT_FEATURE_H

#include <string>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            /**
             * \brief Common interface for orchestration of Device Client features
             *
             * The Feature interface provides some basic methods for orchestrating the individual
             * features running as part of the Device Client (DC). The start() and stop() methods allow
             * the agent base (main.cpp) to either start operation of the feature after performing initialization
             * or to stop a feature in the event that it receives a signal indicating that the program must
             * shut down as soon as possible.
             */
            class Feature
            {
              public:
                virtual ~Feature() = default;

                /**
                 * \brief Status code returned when feature is successfully started or stopped
                 */
                static constexpr int SUCCESS = 0;

                /**
                 * \brief Start the feature
                 *
                 * @return an integer representing the SUCCESS or FAILURE of the start() operation on the feature
                 */
                virtual int start() = 0;

                /**
                 * \brief Stop the feature
                 *
                 * @return an integer representing the SUCCESS or FAILURE of the stop() operation on the feature
                 */
                virtual int stop() = 0;

                /**
                 * \brief For a given feature, returns its name
                 *
                 * @return a string value representing the feature's name
                 */
                virtual std::string getName() = 0;
            };
        } // namespace DeviceClient
    }     // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_FEATURE_H
