#ifndef DEVICE_CLIENT_DEVICEDEFENDERFEATURE_H
#define DEVICE_CLIENT_DEVICEDEFENDERFEATURE_H

#include <aws/iot/MqttClient.h>
#include <aws/iotdevicecommon/IotDevice.h>
#include <aws/iotdevicedefender/DeviceDefender.h>

#include "../ClientBaseNotifier.h"
#include "../Feature.h"
#include "../SharedCrtResourceManager.h"

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            class DeviceDefenderFeature : public Feature
            {
            public:
                int init(
                    std::shared_ptr<SharedCrtResourceManager> manager,
                    std::shared_ptr<ClientBaseNotifier> notifier,
                    const PlainConfig &config);
                void LoadFromConfig(const PlainConfig &config);

                // Interface methods defined in Feature.h
                std::string getName() override;
                int start() override;
                int stop() override;
              private:
                static constexpr char TAG[]= "DeviceDefender.cpp";
                int interval;
                std::string thingName = "";
                std::shared_ptr<SharedCrtResourceManager> resourceManager;
                std::shared_ptr<ClientBaseNotifier> baseNotifier;

                static constexpr char TOPIC_PRE[]= "$aws/things/";
                static constexpr char TOPIC_POST[]= "/defender/metrics/json";
                static constexpr char TOPIC_ACCEPTED[]= "/accepted";
                static constexpr char TOPIC_REJECTED[]= "/rejected";

                std::unique_ptr<Aws::Iotdevicedefenderv1::ReportTask> task;

                void startDeviceDefender();
                void stopDeviceDefender();
            };
        } // namespace DeviceClient
    }     // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_DEVICEDEFENDERFEATURE_H
