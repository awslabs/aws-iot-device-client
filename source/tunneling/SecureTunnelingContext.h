// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_SECURETUNNELINGCONTEXT_H
#define DEVICE_CLIENT_SECURETUNNELINGCONTEXT_H

#include "SecureTunnelWrapper.h"
#include "TcpForward.h"
#include <aws/crt/Types.h>
#include <aws/iotsecuretunneling/SecureTunnel.h>
#include <aws/iotsecuretunneling/SecureTunnelingNotifyResponse.h>
#include <string>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace SecureTunneling
            {
                class SecureTunnelingContext;
                using OnConnectionShutdownFn = std::function<void(SecureTunnelingContext *)>;

                /**
                 * \brief A class that represents a secure tunnel and local TCP port forward pair. The class also
                 * implements all the callbacks required for secure tunneling and local TCP port forward.
                 */
                class SecureTunnelingContext
                {
                  public:
                    /**
                     * \brief Constructor
                     *
                     * @param manager the shared resource manager
                     * @param rootCa path to the Amazon root CA
                     * @param accessToken destination access token for connecting to a secure tunnel
                     * @param endpoint secure tunneling data plain endpoint
                     * @param port the local TCP port to connect to
                     * @param onConnectionShutdown callback when the secure tunnel is shutdown
                     */
                    SecureTunnelingContext(
                        std::shared_ptr<SharedCrtResourceManager> manager,
                        const Aws::Crt::Optional<std::string> &rootCa,
                        const std::string &accessToken,
                        const std::string &endpoint,
                        const int port,
                        const OnConnectionShutdownFn &onConnectionShutdown);

                    SecureTunnelingContext(
                        std::shared_ptr<SharedCrtResourceManager> manager,
                        const Aws::Crt::Http::HttpClientConnectionProxyOptions &proxyOptions,
                        const Aws::Crt::Optional<std::string> &rootCa,
                        const std::string &accessToken,
                        const std::string &endpoint,
                        const int port,
                        const OnConnectionShutdownFn &onConnectionShutdown);

                    /**
                     * \brief Constructor
                     */
                    SecureTunnelingContext() = default;

                    /**
                     * \brief Destructor
                     */
                    virtual ~SecureTunnelingContext();

                    // Non-copyable.
                    SecureTunnelingContext(const SecureTunnelingContext &) = delete;
                    SecureTunnelingContext &operator=(const SecureTunnelingContext &) = delete;

                    /**
                     * \brief Check to see if we have seen and processed the given MQTT notification
                     *
                     * @param response MQTT notification
                     * @return True if the given MQTT notification is a duplication. False otherwise.
                     */
                    virtual bool IsDuplicateNotification(
                        const Aws::Iotsecuretunneling::SecureTunnelingNotifyResponse &response);

                    /**
                     * \brief Connect to a secure tunnel
                     *
                     * @return True if successfully connected to the tunnel. False otherwise.
                     */
                    virtual bool ConnectToSecureTunnel();

                    /**
                     * \brief Stop and close secure tunnel
                     */
                    virtual void StopSecureTunnel();

                  protected:
                    /**
                     * Visible for testing
                     */

                    /**
                     * \brief Callback when data is received from the local TCP port
                     *
                     * @param data data received from the local TCP port
                     */
                    void OnTcpForwardDataReceive(const Crt::ByteBuf &data) const;

                  private:
                    /**
                     * \brief Create a Secure Tunnel instance
                     */
                    virtual std::shared_ptr<SecureTunnelWrapper> CreateSecureTunnel(
                        const Aws::Iotsecuretunneling::OnConnectionComplete &onConnectionComplete,
                        const Aws::Iotsecuretunneling::OnConnectionShutdown &onConnectionShutdown,
                        const Aws::Iotsecuretunneling::OnSendDataComplete &onSendDataComplete,
                        const Aws::Iotsecuretunneling::OnDataReceive &onDataReceive,
                        const Aws::Iotsecuretunneling::OnStreamStart &onStreamStart,
                        const Aws::Iotsecuretunneling::OnStreamReset &onStreamReset,
                        const Aws::Iotsecuretunneling::OnSessionReset &onSessionReset);

                    /**
                     * \brief Create a Tcp Forward instance
                     */
                    virtual std::shared_ptr<TcpForward> CreateTcpForward();

                    /**
                     * \brief Connect to local TCP forward
                     */
                    void ConnectToTcpForward();

                    /**
                     * \brief Disconnect from local TCP forward
                     */
                    virtual void DisconnectFromTcpForward();

                    //
                    // Secure tunneling protocol client callbacks
                    //

                    /**
                     * \brief Callback when secure tunnel connection is complete
                     */
                    void OnConnectionComplete() const;

                    /**
                     * \brief Callback when secure tunnel connection is shutdown
                     */
                    void OnConnectionShutdown();

                    /**
                     * \brief Callback when data send to secure tunnel is complete
                     *
                     * @param errorCode error code
                     */
                    void OnSendDataComplete(int errorCode) const;

                    /**
                     * \brief Callback when data is received from secure tunnel
                     *
                     * @param data data received from the secure tunnel
                     */
                    void OnDataReceive(const Crt::ByteBuf &data) const;

                    /**
                     * \brief Callback when secure tunnel stream_start is received
                     */
                    void OnStreamStart();

                    /**
                     * \brief Callback when secure tunnel stream_reset is received
                     */
                    void OnStreamReset();

                    /**
                     * \brief Callback when secure tunnel session_reset is received
                     */
                    void OnSessionReset();
                    //
                    // Member variables
                    //

                    /**
                     * \brief Used by the logger to specify that log messages are coming from this class
                     */
                    static constexpr char TAG[] = "SecureTunnelingContext.cpp";

                    /**
                     * \brief The resource manager used to manage CRT resources
                     */
                    std::shared_ptr<SharedCrtResourceManager> mSharedCrtResourceManager;

                    /**
                     * \brief HTTP proxy strategy and auth config
                     */
                    Aws::Crt::Http::HttpClientConnectionProxyOptions mProxyOptions;

                    /**
                     * \brief Path to the Amazon root CA
                     */
                    std::string mRootCa;

                    /**
                     * \brief Destination access token
                     */
                    std::string mAccessToken;

                    /**
                     * \brief Secure Tunneling data plain endpoint
                     */
                    std::string mEndpoint;

                    /**
                     * \brief The local TCP port to connect to
                     */
                    uint16_t mPort{22};

                    /**
                     * \brief Callback when the secure tunnel is shutdown
                     */
                    OnConnectionShutdownFn mOnConnectionShutdown;

                    /**
                     * \brief Wrapper around an AWS IoT SDK Secure Tunnel object. It manages the secure tunnel.
                     */
                    std::shared_ptr<SecureTunnelWrapper> mSecureTunnel;

                    /**
                     * \brief Manages local TCP port forward
                     */
                    std::shared_ptr<TcpForward> mTcpForward;

                    /**
                     * \brief Save the MQTT new tunnel notification that results in the creation of this tunnel context.
                     * This is used to avoid creating duplicate tunnel contexts as AWS MQTT Broker may send duplicate
                     * notifications.
                     */
                    Aws::Crt::Optional<Aws::Iotsecuretunneling::SecureTunnelingNotifyResponse> mLastSeenNotifyResponse;
                };
            } // namespace SecureTunneling
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_SECURETUNNELINGCONTEXT_H
