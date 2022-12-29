// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_TCPFORWARD_H
#define DEVICE_CLIENT_TCPFORWARD_H

#include "../SharedCrtResourceManager.h"
#include <aws/crt/Types.h>
#include <aws/io/socket.h>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace SecureTunneling
            {
                // Client callback
                using OnTcpForwardDataReceive = std::function<void(const Crt::ByteBuf &data)>;

                /**
                 * \brief A class that represents a local TCP socket. It implements all callbacks required by using
                 * aws_socket.
                 */
                class TcpForward
                {
                  public:
                    /**
                     * \brief Constructor
                     *
                     * @param sharedCrtResourceManager the shared resource manager
                     * @param port the local TCP port to connect to
                     * @param onTcpForwardDataReceive callback when there is data received from the
                     * local TCP port
                     */
                    TcpForward(
                        std::shared_ptr<SharedCrtResourceManager> sharedCrtResourceManager,
                        uint16_t port,
                        const OnTcpForwardDataReceive &onTcpForwardDataReceive);

                    /**
                     * \brief Constructor with no callback
                     */
                    TcpForward(std::shared_ptr<SharedCrtResourceManager> sharedCrtResourceManager, uint16_t port);

                    /**
                     * \brief Destructor
                     */
                    virtual ~TcpForward();

                    // Non-copyable.
                    TcpForward(const TcpForward &) = delete;
                    TcpForward &operator=(const TcpForward &) = delete;

                    /**
                     * \brief Connect to the local TCP socket
                     */
                    virtual int Connect();

                    /**
                     * \brief Send the given payload to the TCP socket
                     *
                     * @param data the payload to send
                     */
                    virtual int SendData(const Crt::ByteCursor &data);

                  private:
                    //
                    // static callbacks for aws_socket
                    //

                    /**
                     * \brief Callback when connection to a socket is complete
                     */
                    static void sOnConnectionResult(struct aws_socket *socket, int error_code, void *user_data);

                    /**
                     * \brief Callback when writing to the socket is complete
                     */
                    static void sOnWriteCompleted(
                        struct aws_socket *socket,
                        int error_code,
                        size_t bytes_written,
                        void *user_data);

                    /**
                     * \brief Callback when the socket has data to read
                     */
                    static void sOnReadable(struct aws_socket *socket, int error_code, void *user_data);

                    //
                    // Corresponding member callbacks for aws_socket
                    //

                    /**
                     * \brief Callback when connection to a socket is complete
                     */
                    void OnConnectionResult(struct aws_socket *socket, int error_code);

                    /**
                     * \brief Callback when writing to the socket is complete
                     */
                    void OnWriteCompleted(struct aws_socket *socket, int error_code, size_t bytes_written) const;

                    /**
                     * \brief Callback when the socket has data to read
                     */
                    void OnReadable(struct aws_socket *socket, int error_code);

                    /**
                     * \brief Flush any buffered data (saved before the socket is ready) to the socket
                     */
                    void FlushSendBuffer();

                    //
                    // Member data
                    //

                    /**
                     * \brief Used by the logger to specify that log messages are coming from this class
                     */
                    static constexpr char TAG[] = "TcpForward.cpp";

                    /**
                     * \brief The resource manager used to manage CRT resources
                     */
                    std::shared_ptr<SharedCrtResourceManager> mSharedCrtResourceManager;

                    /**
                     * \brief The local TCP port to connect to
                     */
                    uint16_t mPort;

                    /**
                     * \brief Callback when data is received from the local TCP port
                     */
                    OnTcpForwardDataReceive mOnTcpForwardDataReceive;

                    /**
                     * \brief An AWS SDK socket object. It manages the connection to the local TCP port.
                     */
                    aws_socket mSocket{};

                    /**
                     * \brief Is the socket connected yet?
                     */
                    bool mConnected{false};

                    /**
                     * \brief A buffer to store data from the secure tunnel. This is only used before the socket is
                     * connected.
                     */
                    Aws::Crt::ByteBuf mSendBuffer;
                };
            } // namespace SecureTunneling
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_TCPFORWARD_H
