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

                class TcpForward
                {
                  public:
                    TcpForward(
                        std::shared_ptr<SharedCrtResourceManager> sharedCrtResourceManager,
                        uint16_t port,
                        OnTcpForwardDataReceive onTcpForwardDataReceive);
                    ~TcpForward();

                    int Connect();
                    int Close();
                    int SendData(const Crt::ByteCursor &data);

                  private:
                    // static callbacks for aws_socket
                    static void sOnConnectionResult(struct aws_socket *socket, int error_code, void *user_data);
                    static void sOnWriteCompleted(
                        struct aws_socket *socket,
                        int error_code,
                        size_t bytes_written,
                        void *user_data);
                    static void sOnReadable(struct aws_socket *socket, int error_code, void *user_data);

                    // Corresponding member callbacks for aws_socket
                    void OnConnectionResult(struct aws_socket *socket, int error_code);
                    void OnWriteCompleted(struct aws_socket *socket, int error_code, size_t bytes_written);
                    void OnReadable(struct aws_socket *socket, int error_code);

                    // Member data
                    static constexpr char TAG[] = "TcpForward.cpp";

                    std::shared_ptr<SharedCrtResourceManager> mSharedCrtResourceManager;

                    uint16_t mPort;

                    // Client callback
                    OnTcpForwardDataReceive mOnTcpForwardDataReceive;

                    aws_socket mSocket{};
                };
            } // namespace SecureTunneling
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_TCPFORWARD_H