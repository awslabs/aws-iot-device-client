#ifndef DEVICE_CLIENT_SECURETUNNELINGCONTEXT_H
#define DEVICE_CLIENT_SECURETUNNELINGCONTEXT_H

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
                class SecureTunnelingContext
                {
                  public:
                    SecureTunnelingContext(
                        std::shared_ptr<SharedCrtResourceManager> manager,
                        const std::string &rootCa,
                        const std::string &accessToken,
                        const std::string &endpoint,
                        uint16_t port);
                    ~SecureTunnelingContext();

                    bool IsDuplicateNotification(
                        const Aws::Iotsecuretunneling::SecureTunnelingNotifyResponse &response);

                    bool ConnectToSecureTunnel();

                  private:
                    void ConnectToTcpForward();
                    void DisconnectFromTcpForward();

                    // Secure tunneling protocol client callbacks
                    void OnConnectionComplete();
                    void OnSendDataComplete(int errorCode);
                    void OnDataReceive(const Crt::ByteBuf &data);
                    void OnStreamStart();
                    void OnStreamReset();
                    void OnSessionReset();

                    // Tcp forward client callback
                    void OnTcpForwardDataReceive(const Crt::ByteBuf &data);

                    // Member variables
                    static constexpr char TAG[] = "SecureTunnelingContext.cpp";

                    std::shared_ptr<SharedCrtResourceManager> mSharedCrtResourceManager;
                    std::string mRootCa;

                    std::string mAccessToken;
                    std::string mEndpoint;
                    uint16_t mPort{22};

                    std::unique_ptr<Aws::Iotsecuretunneling::SecureTunnel> mSecureTunnel;
                    std::unique_ptr<TcpForward> mTcpForward;

                    Aws::Crt::Optional<Aws::Iotsecuretunneling::SecureTunnelingNotifyResponse> mLastSeenNotifyResponse;
                };
            } // namespace SecureTunneling
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_SECURETUNNELINGCONTEXT_H
