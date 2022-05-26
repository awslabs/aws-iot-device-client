// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_SECURETUNNELINGFEATURE_H
#define DEVICE_CLIENT_SECURETUNNELINGFEATURE_H

#include "../ClientBaseNotifier.h"
#include "../Feature.h"
#include "../SharedCrtResourceManager.h"
#include "IotSecureTunnelingClientWrapper.h"
#include "SecureTunnelingContext.h"
#include <aws/iotdevicecommon/IotDevice.h>
#include <aws/iotsecuretunneling/SecureTunnelingNotifyResponse.h>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace SecureTunneling
            {
                /**
                 * \brief Provides IoT Secure Tunneling related functionality within the Device Client
                 */
                class SecureTunnelingFeature : public Feature
                {
                  public:
                    /**
                     * \brief Constructor
                     */
                    SecureTunnelingFeature();

                    /**
                     * \brief Destructor
                     */
                    virtual ~SecureTunnelingFeature() override;

                    /**
                     * \brief Initializes the Secure Tunneling feature with all the required setup information, event
                     * handlers, and the shared MqttConnection
                     *
                     * @param manager the shared resource manager
                     * @param notifier an ClientBaseNotifier used for notifying the client base of events or errors
                     * @param config configuration information passed in by the user via either the command line or
                     * configuration file
                     * @return a non-zero return code indicates a problem. The logs can be checked for more info
                     */
                    int init(
                        const std::shared_ptr<SharedCrtResourceManager> manager,
                        const std::shared_ptr<ClientBaseNotifier> notifier,
                        const PlainConfig &config);

                    // Interface methods defined in Feature.h
                    std::string getName() override;
                    int start() override;
                    int stop() override;

                    /**
                     * \brief Returns the port number of the given service
                     *
                     * @param service the name of the service
                     * @return the port number
                     */
                    static uint16_t GetPortFromService(const std::string &service);

                    /**
                     * \brief Check if the given port is within the valid range
                     *
                     * @param port the port to check
                     * @return True if the port is within the valid range. False otherwise.
                     */
                    static bool IsValidPort(int port);

                  private:
                    /**
                     * \brief Load configuration data from the config object
                     *
                     * @param config the configuration object to load from
                     */
                    void LoadFromConfig(const PlainConfig &config);

                    /**
                     * \brief Run the Secure Tunneling feature
                     */
                    void RunSecureTunneling();

                    //
                    // MQTT Tunnel Notification
                    //

                    /**
                     * \brief Callback when a MQTT new tunnel notification is received
                     *
                     * @param response MQTT new tunnel notification
                     * @param ioErr error code
                     */
                    void OnSubscribeToTunnelsNotifyResponse(
                        Aws::Iotsecuretunneling::SecureTunnelingNotifyResponse *response,
                        int ioErr);

                    /**
                     * \brief Callback when subscribe to MQTT new tunnel notification is complete
                     *
                     * @param ioErr error code
                     */
                    void OnSubscribeComplete(int ioErr);

                    /**
                     * \brief Get the secure tunneling data plain endpoint given an AWS region
                     *
                     * @param region AWS region
                     * @return Secure Tunneling data plain endpoint
                     */
                    std::string GetEndpoint(const std::string &region);

                    /**
                     * \brief Get the IotSecureTunneling client
                     */
                    virtual std::shared_ptr<AbstractIotSecureTunnelingClient> createClient();

                    /**
                     * \brief a helper function to get SecureTunnelingContext in order to facilitate testing
                     * Pass an empty unique_ptr and set value in order to allow mocking
                     */
                    virtual std::unique_ptr<SecureTunnelingContext> createContext(
                        const std::string &accessToken,
                        const std::string &region,
                        const uint16_t &port);

                    /**
                     * \brief Callback when a secure tunnel is shutdown
                     *
                     * @param contextToRemove a SecureTunnelingContext that represents the secure tunnel that was
                     * shutdown
                     */
                    void OnConnectionShutdown(SecureTunnelingContext *contextToRemove);

                    //
                    // Member variables
                    //

                    /**
                     * \brief Used by the logger to specify that log messages are coming from the Secure Tunneling
                     * feature
                     */
                    static constexpr char TAG[] = "SecureTunnelingFeature.cpp";

                    /**
                     * \brief Format string for forming the secure tunneling data plain endpoint
                     */
                    static constexpr char DEFAULT_PROXY_ENDPOINT_HOST_FORMAT[] = "data.tunneling.iot.%s.amazonaws.com";

                    /**
                     * \brief A map for converting supported services to their port numbers
                     */
                    static std::map<std::string, uint16_t> mServiceToPortMap;

                    /**
                     * \brief The resource manager used to manage CRT resources
                     */
                    std::shared_ptr<SharedCrtResourceManager> mSharedCrtResourceManager;

                    /**
                     * \brief Wrapper around the IotSecureTunnelingClient to facilitate testing
                     */
                    std::shared_ptr<AbstractIotSecureTunnelingClient> iotSecureTunnelingClient;
                    /**
                     * \brief A resource required to initialize the IoT SDK
                     */
                    std::unique_ptr<Aws::Iotdevicecommon::DeviceApiHandle> mDeviceApiHandle;

                    /**
                     * \brief An object used to notify the Client base if there is an event that requires its
                     * attention
                     */
                    std::shared_ptr<ClientBaseNotifier> mClientBaseNotifier;

                    /**
                     * \brief The ThingName to use
                     */
                    std::string mThingName;

                    /**
                     * \brief Path to the Amazon root CA
                     */
                    Aws::Crt::Optional<std::string> mRootCa;

                    /**
                     * \brief Should the Secure Tunneling feature subscribe to MQTT new tunnel notification?
                     */
                    bool mSubscribeNotification{true};

                    /**
                     * \brief Endpoint override. Normally the endpoint is determined by `region` only. This is only used
                     * to override the normal endpoint such as when testing against the gamma stage.
                     */
                    Aws::Crt::Optional<std::string> mEndpoint;

                    /**
                     * \brief A vector of SecureTunnelingContext. Each context represents an active secure tunneling
                     * session.
                     */
                    std::vector<std::unique_ptr<SecureTunnelingContext>> mContexts;
                };
            } // namespace SecureTunneling
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_SECURETUNNELINGFEATURE_H
