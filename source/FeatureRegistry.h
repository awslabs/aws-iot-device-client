// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef AWS_IOT_DEVICE_CLIENT_FEATURE_REGISTRY_H
#define AWS_IOT_DEVICE_CLIENT_FEATURE_REGISTRY_H

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "Feature.h"
#include "config/Config.h"

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Util
            {

                /**
                 * @brief A class to keep track of and manage the features currently running on the device client.
                 */
                class FeatureRegistry
                {
                  public:
                    FeatureRegistry() = default;

                    /**
                     * @brief returns a shared pointer to a Feature, or nullptr if the requested Feature does not exist
                     * in the registry
                     *
                     * @param name The name of the Feature to be retrieved as defined by the Feature's getName() method
                     */
                    std::shared_ptr<Feature> get(const std::string &name) const;

                    /**
                     * @brief Adds a feature to the registry if it does not exist already
                     *
                     * @param feature A shared pointer to the feature to be added
                     */
                    void add(const std::string &name, std::shared_ptr<Feature> feature);

                    /**
                     * @brief Disables a feature in the registry
                     *
                     * @param name The name of the feature to be disabled from the registry as defined by the Feature's
                     * getName() method
                     */
                    void disable(const std::string &name);

                    /**
                     * @brief Returns how many features are currently tracked by the registry
                     *
                     */
                    std::size_t getSize() const;

                    /**
                     * @brief Calls stop() on all features tracked by the registry
                     *
                     */
                    void stopAll();

                    /**
                     * @brief Calls start() on all features tracked by the registry
                     *
                     */
                    void startAll() const;

                  private:
                    static constexpr char TAG[] = "FeatureRegistry.cpp";
                    std::map<std::string, std::shared_ptr<Feature>> features;
                    mutable std::mutex featuresLock;
                };

            } // namespace Util
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // AWS_IOT_DEVICE_CLIENT_FEATURE_REGISTRY_H
