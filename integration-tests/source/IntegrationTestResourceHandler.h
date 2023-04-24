// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <list>
#include <string>

#include <aws/core/Aws.h>
#include <aws/core/utils/ARN.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
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

    /**
     * Secure Tunneling test util methods
     */
    Aws::IoTSecureTunneling::Model::OpenTunnelResult OpenTunnel(const std::string &thingName);

    Aws::IoTSecureTunneling::Model::ConnectionStatus GetTunnelSourceConnectionStatusWithRetry(
        const std::string &tunnelId);

    void CloseTunnel(const std::string &tunnelId);

    /**
     * Jobs test util methods
     */
    void CreateJob(const std::string &jobId, const std::string &jobDoc);

    void CleanUpThingAndCert(const std::string &thingName);

    std::string GetTimeStamp();

    Aws::IoT::Model::JobExecutionStatus GetJobExecutionStatusWithRetry(const std::string &jobId);

    /**
     * Device Defender test util methods
     */
    std::vector<Aws::IoT::Model::ActiveViolation> GetViolations(const std::string &profileName);

    void CreateAndAttachSecurityProfile(
        const std::string &profileName,
        const std::string &thingGroupName,
        const std::vector<std::string> &metrics);

    void DeleteSecurityProfile(const std::string &profileName);

    /**
     * Misc util methods
     */
    void CreateThingGroup(const std::string &thingGroupName);
    void AddThingToThingGroup(const std::string &thingGroupName, const std::string &thingName);
    void CleanUp();
    std::string GetTargetArn(const std::string &thingName);

  protected:
    Aws::IoT::IoTClient iotClient;
    Aws::IoTSecureTunneling::IoTSecureTunnelingClient ioTSecureTunnelingClient;
    std::list<std::string> jobsToCleanUp;
    std::list<std::string> tunnelsToCleanup;
    std::list<std::string> thingGroupsToCleanup;

  private:
    Aws::IoT::Model::JobExecutionStatus GetJobExecutionStatus(const std::string &jobId);
    void DeleteJob(const std::string &jobId);
    std::vector<Aws::Utils::ARN> ListCertsForThing(const std::string &thingName);
    void DetachCertificate(const std::string &thingName, const std::string &certificateArn);
    void DeactivateCertificate(const std::string &certificateId);
    void DeleteCertificate(const std::string &certificateId);
    void DeleteThing(const std::string &thingName);
    std::string GetResourceId(const std::string &resource);
    void AttachSecurityProfile(const std::string &profileName, const std::string &thingGroupName);
    void DeleteThingGroup(const std::string &thingGroupName);
    void Log(
        Aws::Utils::Logging::LogLevel logLevel,
        const std::string &logMessage,
        const std::string &resource,
        const std::string &errorMessage = "");

    std::string targetArn;
    std::unique_ptr<Aws::Utils::Logging::ConsoleLogSystem> logger;
};
