// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_HEARTBEAT_TASK_H
#define DEVICE_CLIENT_HEARTBEAT_TASK_H

#include "../config/Config.h"
#include "SensorState.h"

#include <aws/crt/Types.h>

#include <memory>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace SensorPublish
            {
                /**
                 * \brief HeartbeatTask publishes a heartbeat message while sensor is connected.
                 */
                class HeartbeatTask
                {
                  private:
                    /**
                     * \brief Used by the logger to specify source of log messages.
                     */
                    static constexpr char TAG[] = "HeartbeatTask.cpp";

                    /**
                     * \brief Task for publishing heartbeat to MQTT
                     */
                    aws_task mTask;

                    /**
                     * \brief State machine of the sensor
                     */
                    const SensorState &mState;

                    /**
                     * \brief Settings associated with the sensor
                     */
                    const PlainConfig::SensorPublish::SensorSettings &mSettings;

                    /**
                     * \brief MQTT client connection
                     */
                    std::shared_ptr<Crt::Mqtt::MqttConnection> mConnection;

                    /**
                     * \brief Event loop used to schedule task.
                     */
                    aws_event_loop *mEventLoop{nullptr};

                    /**
                     * \brief Heartbeat topic
                     */
                    aws_byte_cursor mTopic;

                    /**
                     * \brief Heartbeat message payload
                     */
                    aws_byte_cursor mPayload;

                    /**
                     * \brief Flag to indicate task has previously been started
                     */
                    bool mStarted{false};

                    /**
                     * \brief Returns true when heartbeat enabled
                     */
                    bool enabled() const;

                    /**
                     * \brief Publish heartbeat message
                     */
                    void publishHeartbeat();

                    /**
                     * \brief Publish payload to topic
                     */
                    virtual void publish();

                    /**
                     * \brief Schedule next heartbeat message
                     */
                    void scheduleHeartbeat();

                  public:
                    /**
                     * \brief Constructor
                     *
                     * @param state machine of the sensor associated with the heartbeat
                     * @param settings the settings for this sensor
                     * @param connection mqtt connection used to publish heartbeat
                     * @param eventLoop the event loop for the heartbeat
                     */
                    HeartbeatTask(
                        const SensorState &state,
                        const PlainConfig::SensorPublish::SensorSettings &settings,
                        std::shared_ptr<Crt::Mqtt::MqttConnection> connection,
                        aws_event_loop *eventLoop);

                    virtual ~HeartbeatTask() = default;

                    // Non-copyable.
                    HeartbeatTask(const HeartbeatTask &) = delete;
                    HeartbeatTask &operator=(const HeartbeatTask &) = delete;

                    /**
                     * \brief Start publishing heartbeat message
                     *
                     * @return an integer representing the SUCCESS or FAILURE of the start() operation
                     */
                    int start();

                    /**
                     * \brief Stop publishing heartbeat message
                     *
                     * @return an integer representing the SUCCESS or FAILURE of the start() operation
                     */
                    int stop();

                    /**
                     * @return true when heartbeat is started
                     */
                    bool started() const { return mStarted; }
                };
            } // namespace SensorPublish
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_HEARTBEAT_TASK_H
