// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "FeatureRegistry.h"
#include "logging/LoggerFactory.h"

using namespace std;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::Util;
using namespace Aws::Iot::DeviceClient::Logging;

constexpr char FeatureRegistry::TAG[];

std::shared_ptr<Feature> FeatureRegistry::get(const std::string &name) const
{
    std::lock_guard<std::mutex> lock(featuresLock);
    if (features.count(name))
    {
        return features.at(name);
    }
    LOGM_WARN(TAG, "Feature, %s, not found in current registry", name.c_str());
    return nullptr;
}

void FeatureRegistry::add(const std::string &name, std::shared_ptr<Feature> featureToAdd)
{
    std::lock_guard<std::mutex> lock(featuresLock);
    if (!features.count(name))
    {
        features[name] = featureToAdd;
    }
    else
    {
        LOGM_WARN(TAG, "Attemped to add %s to Feature Registry despite it already existing", name.c_str());
    }
}

// NOTE{adanforth} disable is being called in TestFeatureRegistry, and will also be tested later down the line with
// the dynamic reload project
// cppcheck-suppress unusedFunction
void FeatureRegistry::disable(const std::string &name)
{
    std::lock_guard<std::mutex> lock(featuresLock);
    if (features.count(name))
    {
        features[name] = nullptr;
    }
}

std::size_t FeatureRegistry::getSize() const
{
    std::lock_guard<std::mutex> lock(featuresLock);
    return count_if(features.begin(), features.end(), [](const std::pair<string, std::shared_ptr<Feature>> &feature) {
        return feature.second != nullptr;
    });
}

void FeatureRegistry::stopAll()
{
    std::lock_guard<std::mutex> lock(featuresLock);

    for (const auto &feature : features)
    {
        if (feature.second != nullptr)
        {
            LOGM_DEBUG(TAG, "Attempting to stop %s", feature.first.c_str());
            feature.second->stop();
            features[feature.first] = nullptr;
        }
    }
}

void FeatureRegistry::startAll() const
{
    std::lock_guard<std::mutex> lock(featuresLock);

    for (auto &feature : features)
    {
        if (feature.second != nullptr)
        {
            LOGM_DEBUG(TAG, "Attempting to start %s", feature.first.c_str());
            feature.second->start();
        }
    }
}
