#include "SecureTunnelingFeature.h"
#include "../logging/LoggerFactory.h"
#include "TcpForward.h"
#include "aws/iotsecuretunneling/SecureTunnel.h"
#include <aws/crt/mqtt/MqttClient.h>
#include <aws/iotsecuretunneling/IotSecureTunnelingClient.h>
#include <aws/iotsecuretunneling/SubscribeToTunnelsNotifyRequest.h>
#include <map>
#include <memory>
#include <thread>

using namespace std;
using namespace Aws::Iotsecuretunneling;

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace SecureTunneling
            {
                constexpr char SecureTunnelingFeature::TAG[];
                constexpr char SecureTunnelingFeature::DEFAULT_PROXY_ENDPOINT_HOST_FORMAT[];
                std::map<std::string, uint16_t> SecureTunnelingFeature::mServiceToPortMap;

                SecureTunnelingFeature::SecureTunnelingFeature() = default;

                SecureTunnelingFeature::~SecureTunnelingFeature() { aws_http_library_clean_up(); }

                int SecureTunnelingFeature::init(
                    shared_ptr<SharedCrtResourceManager> sharedCrtResourceManager,
                    shared_ptr<ClientBaseNotifier> notifier,
                    Aws::Crt::JsonView dcConfig)
                {
                    // TODO: UA-5774 - Move SDK initialization to application level
                    aws_http_library_init(sharedCrtResourceManager->getAllocator());

                    this->mSharedCrtResourceManager = sharedCrtResourceManager;
                    mDeviceApiHandle = unique_ptr<Aws::Iotdevicecommon::DeviceApiHandle>(
                        new Aws::Iotdevicecommon::DeviceApiHandle(sharedCrtResourceManager->getAllocator()));
                    mClientBaseNotifier = notifier;

                    mThingName = dcConfig.GetString(Config::THING_NAME).c_str();
                    // TODO: Get from config
                    mAccessToken = "";
                    mRegion = "us-west-2";
                    mPort = 22;
                    mSubscribeTunnelNotification = true;

                    return 0;
                }

                string SecureTunnelingFeature::get_name() { return "Secure Tunneling"; }

                int SecureTunnelingFeature::start()
                {
                    runSecureTunneling();
                    mClientBaseNotifier->onEvent((Feature *)this, ClientBaseEventNotification::FEATURE_STARTED);
                    return 0;
                }

                int SecureTunnelingFeature::stop()
                {
                    mSecureTunnel->Close();
                    mSecureTunnel.reset();

                    mTcpForward->Close();
                    mTcpForward.reset();

                    mClientBaseNotifier->onEvent((Feature *)this, ClientBaseEventNotification::FEATURE_STOPPED);
                    return 0;
                }

                void SecureTunnelingFeature::runSecureTunneling()
                {
                    LOGM_INFO(TAG, "Running %s!", get_name().c_str());

                    if (mSubscribeTunnelNotification)
                    {
                        SubscribeToTunnelsNotifyRequest request;
                        request.ThingName = mThingName.c_str();

                        IotSecureTunnelingClient client(mSharedCrtResourceManager->getConnection());
                        client.SubscribeToTunnelsNotify(
                            request,
                            AWS_MQTT_QOS_AT_LEAST_ONCE,
                            bind(
                                &SecureTunnelingFeature::onSubscribeToTunnelsNotifyResponse,
                                this,
                                placeholders::_1,
                                placeholders::_2),
                            bind(&SecureTunnelingFeature::OnSubscribeComplete, this, placeholders::_1));
                    }

                    if (!mAccessToken.empty() && !mRegion.empty() && IsValidPort(mPort))
                    {
                        connectToSecureTunnel(mAccessToken, mRegion);
                        connectToTcpForward(mPort);
                    }
                }

                void SecureTunnelingFeature::onSubscribeToTunnelsNotifyResponse(
                    SecureTunnelingNotifyResponse *response,
                    int ioErr)
                {
                    LOG_DEBUG(TAG, "Received MQTT Tunnel Notification");

                    if (ioErr || !response)
                    {
                        LOGM_ERROR(TAG, "onSubscribeToTunnelsNotifyResponse received error. ioErr=%d", ioErr);
                        return;
                    }

                    string clientAccessToken = response->ClientAccessToken->c_str();
                    string clientMode = response->ClientMode->c_str();
                    string region = response->Region->c_str();

                    size_t nServices = response->Services->size();
                    if (nServices == 0)
                    {
                        LOG_ERROR(TAG, "no service requested");
                        return;
                    }
                    if (nServices > 1)
                    {
                        LOG_ERROR(
                            TAG,
                            "Received a multi-port tunnel request, but multi-port tunneling is not currently supported "
                            "by Device Client.");
                        return;
                    }

                    string service = response->Services->at(0).c_str();
                    LOGM_DEBUG(TAG, "Requested service=%s", service.c_str());
                    connectToSecureTunnel(clientAccessToken, region);
                    connectToTcpForward(GetPortFromService(service));
                }

                void SecureTunnelingFeature::OnSubscribeComplete(int ioErr)
                {
                    if (ioErr)
                    {
                        LOGM_ERROR(TAG, "Couldn't subscribe to tunnel notification topic. ioErr=%d", ioErr);
                        // TODO: Handle subscription error

                        // TODO: UA-5775 - Incorporate the baseClientNotifier onError event
                    }
                }

                void SecureTunnelingFeature::connectToSecureTunnel(const string &accessToken, const string &region)
                {
                    mSecureTunnel = unique_ptr<SecureTunnel>(new SecureTunnel(
                        mSharedCrtResourceManager->getAllocator(),
                        mSharedCrtResourceManager->getClientBootstrap(),
                        Aws::Crt::Io::SocketOptions(),

                        accessToken,
                        AWS_SECURE_TUNNELING_DESTINATION_MODE,
                        GetEndpoint(region),

                        bind(&SecureTunnelingFeature::OnConnectionComplete, this),
                        bind(&SecureTunnelingFeature::OnSendDataComplete, this, placeholders::_1),
                        bind(&SecureTunnelingFeature::OnDataReceive, this, placeholders::_1),
                        bind(&SecureTunnelingFeature::OnStreamStart, this),
                        bind(&SecureTunnelingFeature::OnStreamReset, this),
                        bind(&SecureTunnelingFeature::OnSessionReset, this)));
                    mSecureTunnel->Connect();
                }

                void SecureTunnelingFeature::connectToTcpForward(uint16_t port)
                {
                    mTcpForward = unique_ptr<TcpForward>(new TcpForward(
                        mSharedCrtResourceManager,
                        port,
                        bind(&SecureTunnelingFeature::OnTcpForwardDataReceive, this, placeholders::_1)));
                    mTcpForward->Connect();
                }

                string SecureTunnelingFeature::GetEndpoint(const string &region)
                {
                    string endpoint = FormatMessage(DEFAULT_PROXY_ENDPOINT_HOST_FORMAT, region.c_str());

                    if (region.substr(0, 3) == "cn-")
                    {
                        // Chinese regions have ".cn" at the end:
                        // data.tunneling.iot.<region>.amazonaws.com.cn
                        // Examples of Chinese region name: "cn-north-1", "cn-northwest-1"
                        endpoint = endpoint + ".cn";
                    }

                    return endpoint;
                }

                uint16_t SecureTunnelingFeature::GetPortFromService(const std::string &service)
                {
                    if (mServiceToPortMap.empty())
                    {
                        mServiceToPortMap["SSH"] = 22;
                        mServiceToPortMap["VNC"] = 5900;
                    }

                    auto result = mServiceToPortMap.find(service);
                    if (result == mServiceToPortMap.end())
                    {
                        LOGM_ERROR(TAG, "Requested unsupported service. service=%s", service.c_str());
                        return 0; // TODO: Consider throw
                    }

                    return result->second;
                }

                bool SecureTunnelingFeature::IsValidPort(int port)
                {
                    return 1 <= port && port <= 65535;
                }

                void SecureTunnelingFeature::OnConnectionComplete()
                {
                    LOG_DEBUG(TAG, "SecureTunnelingFeature::OnConnectionComplete");
                }

                void SecureTunnelingFeature::OnSendDataComplete(int errorCode)
                {
                    LOG_DEBUG(TAG, "SecureTunnelingFeature::OnSendDataComplete");
                    if (errorCode)
                    {
                        LOGM_ERROR(TAG, "SecureTunnelingFeature::OnSendDataComplete errorCode=%d", errorCode);
                    }
                }

                void SecureTunnelingFeature::OnDataReceive(const Crt::ByteBuf &data)
                {
                    LOG_DEBUG(TAG, "SecureTunnelingFeature::OnDataReceive");
                    mTcpForward->SendData(aws_byte_cursor_from_buf(&data));
                }

                void SecureTunnelingFeature::OnStreamStart()
                {
                    LOG_DEBUG(TAG, "SecureTunnelingFeature::OnStreamStart");
                }

                void SecureTunnelingFeature::OnStreamReset()
                {
                    LOG_DEBUG(TAG, "SecureTunnelingFeature::OnStreamReset");
                }

                void SecureTunnelingFeature::OnSessionReset()
                {
                    LOG_DEBUG(TAG, "SecureTunnelingFeature::OnSessionReset");
                }

                void SecureTunnelingFeature::OnTcpForwardDataReceive(const Crt::ByteBuf &data)
                {
                    LOG_DEBUG(TAG, "SecureTunnelingFeature::OnTcpForwardDataReceive");
                    mSecureTunnel->SendData(aws_byte_cursor_from_buf(&data));
                }
            } // namespace SecureTunneling
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws