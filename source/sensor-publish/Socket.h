// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_SOCKET_H
#define DEVICE_CLIENT_SOCKET_H

#include <cstddef>

#include <aws/common/zero.h>
#include <aws/io/socket.h>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace SensorPublish
            {
                /**
                 * \brief Socket interface facilitates testing
                 */
                class Socket
                {
                  public:
                    virtual ~Socket() = default;
                    virtual void init(aws_allocator *allocator, aws_socket_options *options) = 0;
                    virtual int connect(
                        const struct aws_socket_endpoint *remote_endpoint,
                        struct aws_event_loop *event_loop,
                        aws_socket_on_connection_result_fn *on_connection_result,
                        void *user_data) = 0;
                    virtual int subscribe_to_readable_events(
                        aws_socket_on_readable_fn *on_readable,
                        void *user_data) = 0;
                    virtual bool is_open() = 0;
                    virtual int read(aws_byte_buf *buf, std::size_t *amount_read) = 0;
                    virtual int close() = 0;
                    virtual void clean_up() = 0;
                };

                /**
                 * \brief AwsSocket wraps aws_socket
                 */
                class AwsSocket : public Socket
                {
                  public:
                    AwsSocket() { AWS_ZERO_STRUCT(socket); }

                    ~AwsSocket() { aws_socket_clean_up(&socket); }

                    // Non-copyable.
                    AwsSocket(const AwsSocket &) = delete;
                    AwsSocket &operator=(const AwsSocket &) = delete;

                    /**
                     * \brief init wraps aws_socket_init
                     */
                    void init(aws_allocator *allocator, aws_socket_options *options) override
                    {
                        AWS_ZERO_STRUCT(socket);
                        aws_socket_init(&socket, allocator, options);
                    }

                    /**
                     * \brief connect wraps aws_socket_connect
                     */
                    int connect(
                        const struct aws_socket_endpoint *remote_endpoint,
                        struct aws_event_loop *event_loop,
                        aws_socket_on_connection_result_fn *on_connection_result,
                        void *user_data) override
                    {
                        return aws_socket_connect(
                            &socket, remote_endpoint, event_loop, on_connection_result, user_data);
                    }

                    /**
                     * \brief subscribe_to_readable_events wraps aws_socket_subscribe_to_readable_events
                     */
                    int subscribe_to_readable_events(aws_socket_on_readable_fn *on_readable, void *user_data) override
                    {
                        return aws_socket_subscribe_to_readable_events(&socket, on_readable, user_data);
                    }

                    /**
                     * \brief is_open wraps aws_socket_is_open
                     */
                    bool is_open() override { return aws_socket_is_open(&socket); }

                    /**
                     * \brief read wraps aws_socket_read
                     */
                    int read(aws_byte_buf *buf, std::size_t *amount_read) override
                    {
                        return aws_socket_read(&socket, buf, amount_read);
                    }

                    /**
                     * \brief close wraps aws_socket_close
                     */
                    int close() override { return aws_socket_close(&socket); }

                    /**
                     * \brief clean_up wraps aws_socket_clean_up
                     */
                    void clean_up() override { aws_socket_clean_up(&socket); }

                  private:
                    /**
                     * \brief Socket for reading sensor data
                     */
                    aws_socket socket{};
                };
            } // namespace SensorPublish
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_SOCKET_H
