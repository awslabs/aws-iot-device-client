// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_SENSOR_H
#define DEVICE_CLIENT_SENSOR_H

#include "../config/Config.h"
#include "HeartbeatTask.h"
#include "SensorState.h"
#include "Socket.h"

#include <aws/crt/Types.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <queue>
#include <regex>
#include <string>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace SensorPublish
            {
                /**
                 * \brief Sensor reads from a sensor and publishes to an MQTT topic.
                 */
                class Sensor
                {
                  protected:
                    /**
                     * \brief Used by the logger to specify source of log messages.
                     */
                    static constexpr char TAG[] = "Sensor.cpp";

                    /**
                     * \brief Settings associated with the sensor
                     */
                    const PlainConfig::SensorPublish::SensorSettings &mSettings;

                    /**
                     * \brief Memory allocator
                     */
                    aws_allocator *mAllocator{nullptr};

                    /**
                     * \brief MQTT client connection
                     */
                    std::shared_ptr<Crt::Mqtt::MqttConnection> mConnection;

                    /**
                     * \brief Reading and publishing are managed through the same event loop
                     *
                     * Event loop is edge triggered.
                     */
                    aws_event_loop *mEventLoop{nullptr};

                    /**
                     * \brief Socket for reading sensor data
                     */
                    std::shared_ptr<Socket> mSocket;

                    /**
                     * \brief Buffer for reading sensor data
                     *
                     * Buffer is allocated once and never larger than AWS IoT maximum message size.
                     */
                    aws_byte_buf mReadBuf;

                    /**
                     * \brief End of message boundaries in read buffer
                     *
                     * Stores index in buffer of one-past the end of the boundary.
                     */
                    std::queue<size_t> mEomBounds;

                    /**
                     * \brief Pattern used to identify end of message boundary
                     */
                    std::regex mEomPattern;

                    using TimePointT = std::chrono::high_resolution_clock::time_point;

                    /**
                     * \brief MQTT topic
                     */
                    aws_byte_cursor mTopic;

                    /**
                     * \brief Absolute time after which next batch must be published
                     */
                    TimePointT mNextPublishTimeout;

                    /**
                     * \brief State machine for the Sensor
                     */
                    SensorState mState{SensorState::NotConnected};

                    /**
                     * \brief Task for publishing heartbeat to MQTT
                     */
                    HeartbeatTask mHeartbeatTask;

                    /**
                     * \brief Task for connecting to sensor
                     */
                    aws_task mConnectTask;

                    /**
                     * \brief Connect to the sensor
                     */
                    virtual void connect(bool delay = false);

                    /**
                     * \brief Callback function for connect task
                     */
                    void onConnectTaskCallback();

                    /**
                     * \brief Callback function for connect
                     */
                    void onConnectionResultCallback(int error_code);

                    /**
                     * \brief Callback function when sensor data available for read
                     */
                    void onReadableCallback(int error_code);

                    /**
                     * \brief Publish buffered messages
                     */
                    virtual void publish();

                    /**
                     * \brief Check whether publish limits are breached
                     *
                     * @param bufferSize is set on return from function
                     * @param numBatches is set on return from function
                     *
                     * @return true when we will publish at least one batch
                     */
                    bool needPublish(size_t &bufferSize, size_t &numBatches);

                    /**
                     * \brief Publish one message
                     */
                    void publishOneMessage(const aws_byte_cursor *payload);

                    /**
                     * \brief Close connection to server
                     */
                    virtual void close();

                    /**
                     * \brief Reset read state
                     */
                    void reset();

                  public:
                    /**
                     * \brief Constructor
                     *
                     * @param settings the settings for this sensor
                     */
                    Sensor(
                        const PlainConfig::SensorPublish::SensorSettings &settings,
                        aws_allocator *allocator,
                        std::shared_ptr<Crt::Mqtt::MqttConnection> connection,
                        aws_event_loop *eventLoop,
                        std::shared_ptr<Socket> socket);

                    virtual ~Sensor();

                    // Non-copyable.
                    Sensor(const Sensor &) = delete;
                    Sensor &operator=(const Sensor &) = delete;

                    /**
                     * \brief Start reading from sensor
                     *
                     * @return an integer representing the SUCCESS or FAILURE of the start() operation
                     */
                    virtual int start();

                    /**
                     * \brief Stop reading from sensor
                     *
                     * @return an integer representing the SUCCESS or FAILURE of the stop() operation
                     */
                    virtual int stop();

                    /**
                     * \brief Sensor name
                     *
                     * @return a string value representing the sensor name
                     */
                    std::string getName() const;
                };
            } // namespace SensorPublish
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_SENSOR_H
