#ifndef DEVICE_CLIENT_SECURETUNNELINGFEATURE_H
#define DEVICE_CLIENT_SECURETUNNELINGFEATURE_H

#include "../ClientBaseNotifier.h"
#include "../Feature.h"
#include "../SharedCrtResourceManager.h"
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
                class SecureTunnelingContext;

                class SecureTunnelingFeature : public Feature
                {
                  public:
                    SecureTunnelingFeature();
                    ~SecureTunnelingFeature() override;

                    int init(
                        std::shared_ptr<SharedCrtResourceManager> manager,
                        std::shared_ptr<ClientBaseNotifier> notifier,
                        const PlainConfig &config);

                    // Interface methods defined in Feature.h
                    std::string getName() override;
                    int start() override;
                    int stop() override;

                    static uint16_t GetPortFromService(const std::string &service);
                    static bool IsValidPort(int port);

                  private:
                    void LoadFromConfig(const PlainConfig &config);

                    void runSecureTunneling();

                    // MQTT Tunnel Notification
                    void onSubscribeToTunnelsNotifyResponse(
                        Aws::Iotsecuretunneling::SecureTunnelingNotifyResponse *response,
                        int ioErr);
                    void OnSubscribeComplete(int ioErr);

                    /*
                    void connectToSecureTunnel(const std::string &accessToken, const std::string &region);
                    void connectToTcpForward(uint16_t port);
                    void disconnectFromTcpForward();
                    */

                    std::string GetEndpoint(const std::string &region);

                    /*
                                        // Secure tunneling protocol client callbacks
                                        void OnConnectionComplete();
                                        void OnSendDataComplete(int errorCode);
                                        void OnDataReceive(const Crt::ByteBuf &data);
                                        void OnStreamStart();
                                        void OnStreamReset();
                                        void OnSessionReset();

                                        // Tcp forward client callback
                                        void OnTcpForwardDataReceive(const Crt::ByteBuf &data);
                    */

                    // Member variables
                    static constexpr char TAG[] = "SecureTunneling.cpp";
                    static constexpr char DEFAULT_PROXY_ENDPOINT_HOST_FORMAT[] = "data.tunneling.iot.%s.amazonaws.com";
                    static std::map<std::string, uint16_t> mServiceToPortMap;

                    std::shared_ptr<SharedCrtResourceManager> mSharedCrtResourceManager;
                    std::unique_ptr<Aws::Iotdevicecommon::DeviceApiHandle> mDeviceApiHandle;
                    std::shared_ptr<ClientBaseNotifier> mClientBaseNotifier;

                    std::string mThingName;
                    std::string mRootCa;
                    bool mSubscribeNotification{true};

                    // Normally the endpoint is determined by `region` only. This is only used to override the normal
                    // endpoint such as when testing against the gamma stage.
                    Aws::Crt::Optional<std::string> mEndpoint;

                    // Moving to SecureTunnelingContext
                    /*
                                        // Per tunnel
                                        std::string mAccessToken;
                                        std::string mRegion;
                                        uint16_t mPort{22};

                                        std::unique_ptr<Aws::Iotsecuretunneling::SecureTunnel> mSecureTunnel;
                                        std::unique_ptr<TcpForward> mTcpForward;

                                        Aws::Crt::Optional<Aws::Iotsecuretunneling::SecureTunnelingNotifyResponse>
                       mLastSeenNotifyResponse;
                    */

                    std::vector<std::unique_ptr<SecureTunnelingContext>> mContexts;
                };
            } // namespace SecureTunneling
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_SECURETUNNELINGFEATURE_H
