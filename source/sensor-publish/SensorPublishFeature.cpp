// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "SensorPublishFeature.h"

#include "../logging/LoggerFactory.h"

#include <aws/common/error.h>

#include <stdexcept>

using namespace std;
using namespace Aws::Iot;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::Logging;
using namespace Aws::Iot::DeviceClient::SensorPublish;

const int Feature::SUCCESS; // Retain reference from header-only source to avoid linker errors.

constexpr char SensorPublishFeature::TAG[];
constexpr char SensorPublishFeature::NAME[];

int SensorPublishFeature::init(
    shared_ptr<SharedCrtResourceManager> manager,
    shared_ptr<ClientBaseNotifier> notifier,
    const PlainConfig &config)
{
    mResourceManager = manager;
    mBaseNotifier = notifier;

    for (auto &setting : config.sensorPublish.settings)
    {
        if (setting.enabled)
        {
            try
            {
                auto *eventLoop = mResourceManager->getNextEventLoop();
                if (eventLoop)
                {
                    mSensors.emplace_back(createSensor(
                        setting, mResourceManager->getAllocator(), mResourceManager->getConnection(), eventLoop));
                }
                else
                {
                    throw std::runtime_error{"event loop returned by crt is null"};
                }
            }
            catch (const std::exception &e)
            {
                LOGM_ERROR(TAG, "Error initializing sensor: %s message: %s", setting.name->c_str(), e.what());
            }
        }
    }

    return Feature::SUCCESS;
}

std::unique_ptr<Sensor> SensorPublishFeature::createSensor(
    const PlainConfig::SensorPublish::SensorSettings &settings,
    aws_allocator *allocator,
    std::shared_ptr<Crt::Mqtt::MqttConnection> connection,
    aws_event_loop *eventLoop) const
{
    return std::unique_ptr<Sensor>(
        new Sensor(settings, allocator, connection, eventLoop, std::make_shared<AwsSocket>()));
}

std::string SensorPublishFeature::getName()
{
    return NAME;
}

// cppcheck-suppress unusedFunction
std::size_t SensorPublishFeature::getSensorsSize() const
{
    return mSensors.size();
}

int SensorPublishFeature::start()
{
    LOGM_INFO(TAG, "Starting %s", getName().c_str());

    for (auto &sensor : mSensors)
    {
        if (sensor->start() != SharedCrtResourceManager::SUCCESS)
        {
            LOGM_INFO(TAG, "Failed to start sensor: %s", sensor->getName().c_str());
        }
    }

    mBaseNotifier->onEvent(static_cast<Feature *>(this), ClientBaseEventNotification::FEATURE_STARTED);

    return Feature::SUCCESS;
}

int SensorPublishFeature::stop()
{
    LOGM_INFO(TAG, "Stopping %s", getName().c_str());

    for (auto &sensor : mSensors)
    {
        if (sensor->stop() != SharedCrtResourceManager::SUCCESS)
        {
            LOGM_INFO(TAG, "Failed to stop sensor: %s", sensor->getName().c_str());
        }
    }

    mBaseNotifier->onEvent(static_cast<Feature *>(this), ClientBaseEventNotification::FEATURE_STOPPED);

    return Feature::SUCCESS;
}
