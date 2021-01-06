#include "SecureTunnelingContext.h"
#include "../logging/LoggerFactory.h"
#include "SecureTunnelingFeature.h"
#include <aws/iotsecuretunneling/SecureTunnel.h>

using namespace std;
using namespace Aws::Iotsecuretunneling;
using namespace Aws::Iot::DeviceClient::Logging;

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace SecureTunneling
            {
                constexpr char SecureTunnelingContext::TAG[];

                SecureTunnelingContext::SecureTunnelingContext(
                    std::shared_ptr<SharedCrtResourceManager> manager,
                    const std::string &rootCa,
                    const std::string &accessToken,
                    const std::string &endpoint,
                    uint16_t port)
                {
                    mSharedCrtResourceManager = manager;
                    mRootCa = rootCa;
                    mAccessToken = accessToken;
                    mEndpoint = endpoint;
                    mPort = port;
                }

                SecureTunnelingContext::~SecureTunnelingContext()
                {
                    mSecureTunnel->Close();
                    mTcpForward->Close();
                }

                template <typename T>
                static bool operator==(const Aws::Crt::Optional<T> &lhs, const Aws::Crt::Optional<T> &rhs)
                {
                    if (!lhs.has_value() && !rhs.has_value())
                    {
                        return true;
                    }
                    else if (lhs.has_value() && rhs.has_value())
                    {
                        return lhs.value() == rhs.value();
                    }
                    else
                    {
                        return false;
                    }
                }

                static bool operator==(
                    const SecureTunnelingNotifyResponse &lhs,
                    const SecureTunnelingNotifyResponse &rhs)
                {
                    return lhs.Region == rhs.Region && lhs.ClientMode == rhs.ClientMode &&
                           lhs.Services == rhs.Services && lhs.ClientAccessToken == rhs.ClientAccessToken;
                }

                bool SecureTunnelingContext::IsDuplicateNotification(
                    const Aws::Iotsecuretunneling::SecureTunnelingNotifyResponse &response)
                {
                    if (mLastSeenNotifyResponse.has_value() && mLastSeenNotifyResponse.value() == response)
                    {
                        return true;
                    }

                    mLastSeenNotifyResponse = response;
                    return false;
                }

                void SecureTunnelingContext::ConnectToSecureTunnel()
                {
                    if (mAccessToken.empty() || mEndpoint.empty())
                    {
                        LOG_ERROR(TAG, "Cannot connect to secure tunnel. Either access token or endpoint is empty");
                        return;
                    }

                    mSecureTunnel = unique_ptr<SecureTunnel>(new SecureTunnel(
                        mSharedCrtResourceManager->getAllocator(),
                        mSharedCrtResourceManager->getClientBootstrap(),
                        Aws::Crt::Io::SocketOptions(),

                        mAccessToken,
                        AWS_SECURE_TUNNELING_DESTINATION_MODE,
                        mEndpoint,
                        mRootCa,

                        bind(&SecureTunnelingContext::OnConnectionComplete, this),
                        bind(&SecureTunnelingContext::OnSendDataComplete, this, placeholders::_1),
                        bind(&SecureTunnelingContext::OnDataReceive, this, placeholders::_1),
                        bind(&SecureTunnelingContext::OnStreamStart, this),
                        bind(&SecureTunnelingContext::OnStreamReset, this),
                        bind(&SecureTunnelingContext::OnSessionReset, this)));

                    mSecureTunnel->Connect();
                }

                void SecureTunnelingContext::ConnectToTcpForward()
                {
                    if (!SecureTunnelingFeature::IsValidPort(mPort))
                    {
                        LOGM_ERROR(TAG, "Cannot connect to invalid local port. port=%d", mPort);
                        return;
                    }

                    mTcpForward = unique_ptr<TcpForward>(new TcpForward(
                        mSharedCrtResourceManager,
                        mPort,
                        bind(&SecureTunnelingContext::OnTcpForwardDataReceive, this, placeholders::_1)));
                    mTcpForward->Connect();
                }

                void SecureTunnelingContext::DisconnectFromTcpForward()
                {
                    mTcpForward->Close();
                    mTcpForward.reset();
                }

                void SecureTunnelingContext::OnConnectionComplete()
                {
                    LOG_DEBUG(TAG, "SecureTunnelingFeature::OnConnectionComplete");
                }

                void SecureTunnelingContext::OnSendDataComplete(int errorCode)
                {
                    LOG_DEBUG(TAG, "SecureTunnelingFeature::OnSendDataComplete");
                    if (errorCode)
                    {
                        LOGM_ERROR(TAG, "SecureTunnelingFeature::OnSendDataComplete errorCode=%d", errorCode);
                    }
                }

                void SecureTunnelingContext::OnDataReceive(const Crt::ByteBuf &data)
                {
                    LOGM_DEBUG(TAG, "SecureTunnelingFeature::OnDataReceive data.len=%d", data.len);
                    mTcpForward->SendData(aws_byte_cursor_from_buf(&data));
                }

                void SecureTunnelingContext::OnStreamStart()
                {
                    LOG_DEBUG(TAG, "SecureTunnelingFeature::OnStreamStart");
                    ConnectToTcpForward();
                }

                void SecureTunnelingContext::OnStreamReset()
                {
                    LOG_DEBUG(TAG, "SecureTunnelingFeature::OnStreamReset");
                    DisconnectFromTcpForward();
                }

                void SecureTunnelingContext::OnSessionReset()
                {
                    LOG_DEBUG(TAG, "SecureTunnelingFeature::OnSessionReset");
                    DisconnectFromTcpForward();
                }

                void SecureTunnelingContext::OnTcpForwardDataReceive(const Crt::ByteBuf &data)
                {
                    LOGM_DEBUG(TAG, "SecureTunnelingFeature::OnTcpForwardDataReceive data.len=%d", data.len);
                    mSecureTunnel->SendData(aws_byte_cursor_from_buf(&data));
                }

            } // namespace SecureTunneling
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws
