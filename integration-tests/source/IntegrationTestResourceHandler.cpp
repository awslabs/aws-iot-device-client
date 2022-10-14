// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "IntegrationTestResourceHandler.h"
#include <aws/iot/model/CreateJobRequest.h>
#include <aws/iot/model/CreateJobResult.h>
#include <aws/iot/model/DeleteCertificateRequest.h>
#include <aws/iot/model/DeleteJobRequest.h>
#include <aws/iot/model/DeleteThingGroupRequest.h>
#include <aws/iot/model/DeleteThingRequest.h>
#include <aws/iot/model/DescribeCertificateRequest.h>
#include <aws/iot/model/DescribeJobExecutionRequest.h>
#include <aws/iot/model/DescribeJobExecutionResult.h>
#include <aws/iot/model/DescribeThingRequest.h>
#include <aws/iot/model/DetachPolicyRequest.h>
#include <aws/iot/model/DetachThingPrincipalRequest.h>
#include <aws/iot/model/ListAttachedPoliciesRequest.h>
#include <aws/iot/model/ListJobExecutionsForJobRequest.h>
#include <aws/iot/model/ListThingPrincipalsRequest.h>
#include <aws/iot/model/ListThingPrincipalsResult.h>
#include <aws/iot/model/ListThingsInThingGroupRequest.h>
#include <aws/iot/model/UpdateCertificateRequest.h>
#include <aws/iotsecuretunneling/model/CloseTunnelRequest.h>
#include <aws/iotsecuretunneling/model/DescribeTunnelRequest.h>
#include <aws/iotsecuretunneling/model/DescribeTunnelResult.h>
#include <aws/iotsecuretunneling/model/OpenTunnelRequest.h>

#include <thread>

using namespace std;
using namespace Aws;
using namespace Aws::IoTSecureTunneling;
using namespace Aws::IoTSecureTunneling::Model;
using namespace Aws::Client;
using namespace Aws::IoT;
using namespace Aws::IoT::Model;

extern std::string THING_NAME;

IntegrationTestResourceHandler::IntegrationTestResourceHandler(const ClientConfiguration &clientConfig)
    : iotClient(IoTClient(clientConfig)), ioTSecureTunnelingClient(IoTSecureTunnelingClient(clientConfig))
{
    targetArn = GetTargetArn(THING_NAME);
}
void IntegrationTestResourceHandler::CreateJob(const string &jobId, const string &jobDoc)
{
    CreateJobRequest request;
    request.SetJobId(jobId);
    request.SetDocument(jobDoc);
    request.AddTargets(targetArn);

    CreateJobOutcome outcome = iotClient.CreateJob(request);

    if (outcome.IsSuccess())
    {
        jobsToCleanUp.push_back(jobId);
    }
    else
    {
        printf("Failed to create Job: %s\n", outcome.GetError().GetMessage().c_str());
    }
}

JobExecutionStatus IntegrationTestResourceHandler::GetJobExecutionStatus(const std::string &jobId)
{
    DescribeJobExecutionRequest request;
    request.SetJobId(jobId);
    request.SetThingName(THING_NAME);

    DescribeJobExecutionOutcome outcome = iotClient.DescribeJobExecution(request);

    if (!outcome.IsSuccess())
    {
        printf(
            "Failed to describe job execution for Job: %s\n%s\n",
            jobId.c_str(),
            outcome.GetError().GetMessage().c_str());
    }
    return outcome.GetResult().GetExecution().GetStatus();
}
void IntegrationTestResourceHandler::DeleteJob(const std::string &jobId)
{
    DeleteJobRequest request;
    request.SetJobId(jobId);
    request.SetForce(true);

    DeleteJobOutcome outcome = iotClient.DeleteJob(request);
    if (!outcome.IsSuccess())
    {
        printf("Failed to delete Job: %s\n%s\n", jobId.c_str(), outcome.GetError().GetMessage().c_str());
    }
}
void IntegrationTestResourceHandler::DeleteThing(const std::string &thingName)
{
    DeleteThingRequest request;
    request.SetThingName(thingName);

    DeleteThingOutcome outcome = iotClient.DeleteThing(request);

    if (!outcome.IsSuccess())
    {
        printf("Failed to delete Thing: %s\n%s\n", thingName.c_str(), outcome.GetError().GetMessage().c_str());
    }
}

vector<Aws::Utils::ARN> IntegrationTestResourceHandler::ListCertsForThing(const string &thingName)
{
    vector<Aws::Utils::ARN> certs;

    ListThingPrincipalsRequest request;
    request.SetThingName(thingName);

    ListThingPrincipalsOutcome outcome = iotClient.ListThingPrincipals(request);

    if (outcome.IsSuccess())
    {
        ListThingPrincipalsResult result = outcome.GetResult();
        certs.insert(certs.begin(), result.GetPrincipals().begin(), result.GetPrincipals().end());
    }
    else
    {
        printf(
            "Failed to list Certificates for Thing: %s\n%s\n",
            thingName.c_str(),
            outcome.GetError().GetMessage().c_str());
    }

    return certs;
}

void IntegrationTestResourceHandler::DeactivateCertificate(const string &certificateId)
{
    UpdateCertificateRequest request;
    request.SetCertificateId(certificateId);
    request.SetNewStatus(CertificateStatus::INACTIVE);

    UpdateCertificateOutcome outcome = iotClient.UpdateCertificate(request);

    if (!outcome.IsSuccess())
    {
        printf(
            "Failed to deactivate Certificate: %s\n%s\n",
            certificateId.c_str(),
            outcome.GetError().GetMessage().c_str());
    }
}

void IntegrationTestResourceHandler::DetachCertificate(const string &thingName, const string &certificateArn)
{
    DetachThingPrincipalRequest request;
    request.SetThingName(thingName);
    request.SetPrincipal(certificateArn);

    DetachThingPrincipalOutcome outcome = iotClient.DetachThingPrincipal(request);

    if (!outcome.IsSuccess())
    {
        printf("Failed to detach Certificate: %s\n", outcome.GetError().GetMessage().c_str());
    }
}

void IntegrationTestResourceHandler::DeleteCertificate(const string &certificateId)
{
    DeleteCertificateRequest request;
    request.SetCertificateId(certificateId);
    request.SetForceDelete(true);

    DeleteCertificateOutcome outcome = iotClient.DeleteCertificate(request);

    if (!outcome.IsSuccess())
    {
        printf(
            "Failed to delete Certificate: %s\n%s\n", certificateId.c_str(), outcome.GetError().GetMessage().c_str());
    }
}

void IntegrationTestResourceHandler::CleanUp()
{
    for (const string &job : jobsToCleanUp)
    {
        DeleteJob(job);
        this_thread::sleep_for(chrono::milliseconds(200));
    }
    for (const string &tunnelId : tunnelsToCleanup)
    {
        CloseTunnel(tunnelId);
    }
}

void IntegrationTestResourceHandler::CleanUpThingAndCert(const std::string &thingName)
{
    vector<Aws::Utils::ARN> certificates = ListCertsForThing(thingName);

    for (const Aws::Utils::ARN &certificate : certificates)
    {
        string certificateArn = certificate.GetARNString();
        string certificateId = GetResourceId(certificate.GetResource());

        DetachCertificate(thingName, certificateArn);
        DeactivateCertificate(certificateId);
        DeleteCertificate(certificateId);
        DeleteThing(thingName);
    }
}

std::string IntegrationTestResourceHandler::GetTimeStamp()
{
    time_t t;
    struct tm *ptm;
    // UTC time
    time(&t);
    ptm = gmtime(&t);
    return to_string(mktime(ptm));
}

JobExecutionStatus IntegrationTestResourceHandler::GetJobExecutionStatusWithRetry(const string &jobId)
{
    int retries = 3;
    int backoff = 30;
    JobExecutionStatus status;
    do
    {
        this_thread::sleep_for(chrono::seconds(backoff));
        status = GetJobExecutionStatus(jobId);
        retries--;
        backoff *= 2;
    } while (status == JobExecutionStatus::IN_PROGRESS && retries > 0);

    if (status == JobExecutionStatus::IN_PROGRESS)
    {
        printf("JobExecution for Job  %s still IN_PROGRESS state after max retries \n", jobId.c_str());
    }

    return status;
}

ConnectionStatus IntegrationTestResourceHandler::GetTunnelSourceConnectionStatusWithRetry(const string &tunnelId)
{
    ConnectionStatus status = ConnectionStatus::NOT_SET;
    int retries = 3;
    int backoff = 10;
    do
    {
        this_thread::sleep_for(chrono::seconds(backoff));
        retries--;
        backoff *= 2;

        DescribeTunnelRequest request;
        request.SetTunnelId(tunnelId);

        DescribeTunnelOutcome outcome = ioTSecureTunnelingClient.DescribeTunnel(request);

        if (!outcome.IsSuccess())
        {
            printf("Failed to describe Tunnel: %s\n%s\n", tunnelId.c_str(), outcome.GetError().GetMessage().c_str());
        }
        else
        {
            status = outcome.GetResult().GetTunnel().GetSourceConnectionState().GetStatus();
        }
    } while (status != ConnectionStatus::CONNECTED && retries > 0);

    return status;
}

Aws::IoTSecureTunneling::Model::OpenTunnelResult IntegrationTestResourceHandler::OpenTunnel(
    const std::string &thingName)
{
    Aws::IoTSecureTunneling::Model::OpenTunnelResult result;

    Aws::Vector<Aws::String> services;
    services.push_back("SSH");

    DestinationConfig destinationConfig;
    destinationConfig.SetServices(services);
    destinationConfig.SetThingName(thingName);

    OpenTunnelRequest request;
    request.SetDestinationConfig(destinationConfig);

    OpenTunnelOutcome outcome = ioTSecureTunnelingClient.OpenTunnel(request);

    if (!outcome.IsSuccess())
    {
        printf("Failed to open tunnel to Thing: %s\n%s\n", thingName.c_str(), outcome.GetError().GetMessage().c_str());
    }
    else
    {
        result = outcome.GetResult();
    }
    return result;
}

void IntegrationTestResourceHandler::CloseTunnel(const string &tunnelId)
{
    CloseTunnelRequest request;
    request.SetTunnelId(tunnelId);

    ioTSecureTunnelingClient.CloseTunnel(request);
}

std::string IntegrationTestResourceHandler::GetTargetArn(const std::string &target)
{
    DescribeThingRequest request;
    request.SetThingName(target);
    return iotClient.DescribeThing(request).GetResult().GetThingArn();
}
std::string IntegrationTestResourceHandler::GetResourceId(const std::string &resource)
{
    return resource.substr(resource.find('/'));
}
