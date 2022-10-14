// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <list>
#include <string>

#include <aws/core/utils/ARN.h>
#include <aws/iot/IoTClient.h>
#include <aws/iot/model/JobExecutionStatus.h>
#include <aws/iotsecuretunneling/IoTSecureTunnelingClient.h>
#include <aws/iotsecuretunneling/model/ConnectionStatus.h>
#include <aws/iotsecuretunneling/model/OpenTunnelResult.h>
#include <gtest/gtest.h>

class IntegrationTestResourceHandler
{
  public:
    explicit IntegrationTestResourceHandler(const Aws::Client::ClientConfiguration &clientConfig);
    Aws::IoTSecureTunneling::Model::OpenTunnelResult OpenTunnel(const std::string &thingName);
    Aws::IoTSecureTunneling::Model::ConnectionStatus GetTunnelSourceConnectionStatusWithRetry(
        const std::string &tunnelId);
    void CloseTunnel(const std::string &tunnelId);
    void CreateJob(const std::string &jobId, const std::string &jobDoc);

    void CleanUp();
    void CleanUpThingAndCert(const std::string &thingName);
    std::string GetTimeStamp();
    Aws::IoT::Model::JobExecutionStatus GetJobExecutionStatusWithRetry(const std::string &jobId);
    std::string GetTargetArn(const std::string &target);

  protected:
    Aws::IoT::IoTClient iotClient;
    Aws::IoTSecureTunneling::IoTSecureTunnelingClient ioTSecureTunnelingClient;
    std::list<std::string> jobsToCleanUp;
    std::list<std::string> tunnelsToCleanup;

  private:
    Aws::IoT::Model::JobExecutionStatus GetJobExecutionStatus(const std::string &jobId);
    void DeleteJob(const std::string &jobId);
    std::vector<Aws::Utils::ARN> ListCertsForThing(const std::string &thingName);
    void DetachCertificate(const std::string &thingName, const std::string &certificateArn);
    void DeactivateCertificate(const std::string &certificateId);
    void DeleteCertificate(const std::string &certificateId);
    void DeleteThing(const std::string &thingName);
    std::string GetResourceId(const std::string &resource);

    std::string targetArn;
};
