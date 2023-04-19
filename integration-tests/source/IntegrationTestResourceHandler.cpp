// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "IntegrationTestResourceHandler.h"
#include <aws/core/Aws.h>
#include <aws/iot/model/AddThingToThingGroupRequest.h>
#include <aws/iot/model/AddThingToThingGroupResult.h>
#include <aws/iot/model/AttachSecurityProfileRequest.h>
#include <aws/iot/model/CreateJobRequest.h>
#include <aws/iot/model/CreateJobResult.h>
#include <aws/iot/model/CreateSecurityProfileRequest.h>
#include <aws/iot/model/CreateSecurityProfileResult.h>
#include <aws/iot/model/CreateThingGroupRequest.h>
#include <aws/iot/model/CreateThingGroupResult.h>
#include <aws/iot/model/DeleteCertificateRequest.h>
#include <aws/iot/model/DeleteJobRequest.h>
#include <aws/iot/model/DeleteSecurityProfileRequest.h>
#include <aws/iot/model/DeleteThingGroupRequest.h>
#include <aws/iot/model/DeleteThingGroupResult.h>
#include <aws/iot/model/DeleteThingRequest.h>
#include <aws/iot/model/DescribeCertificateRequest.h>
#include <aws/iot/model/DescribeJobExecutionRequest.h>
#include <aws/iot/model/DescribeJobExecutionResult.h>
#include <aws/iot/model/DescribeThingRequest.h>
#include <aws/iot/model/DescribeThingResult.h>
#include <aws/iot/model/DetachPolicyRequest.h>
#include <aws/iot/model/DetachThingPrincipalRequest.h>
#include <aws/iot/model/ListActiveViolationsRequest.h>
#include <aws/iot/model/ListActiveViolationsResult.h>
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
using namespace Aws::Utils;

extern std::string THING_NAME;

static const char *TAG = "IntegrationTestResourceHandler.cpp";

IntegrationTestResourceHandler::IntegrationTestResourceHandler(const ClientConfiguration &clientConfig)
    : iotClient(IoTClient(clientConfig)), ioTSecureTunnelingClient(IoTSecureTunnelingClient(clientConfig))
{
    targetArn = GetTargetArn(THING_NAME);
    logger = unique_ptr<Aws::Utils::Logging::ConsoleLogSystem>(
        new Aws::Utils::Logging::ConsoleLogSystem(Aws::Utils::Logging::LogLevel::Info));
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
        Log(Logging::LogLevel::Error, "Failed to Create Job", jobId, outcome.GetError().GetMessage());
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
        Log(Logging::LogLevel::Error,
            "Failed to describe job execution for Job",
            jobId,
            outcome.GetError().GetMessage());
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
        Log(Logging::LogLevel::Error, "Failed to Delete Job", jobId, outcome.GetError().GetMessage());
    }
}
void IntegrationTestResourceHandler::DeleteThing(const std::string &thingName)
{
    DeleteThingRequest request;
    request.SetThingName(thingName);

    DeleteThingOutcome outcome = iotClient.DeleteThing(request);

    if (!outcome.IsSuccess())
    {
        Log(Logging::LogLevel::Error, "Failed to delete Thing", thingName, outcome.GetError().GetMessage());
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
        Log(Logging::LogLevel::Error,
            "Failed to list Certificates for Thing",
            thingName,
            outcome.GetError().GetMessage());
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
        Log(Logging::LogLevel::Error,
            "Failed to de-activate Certificate",
            certificateId,
            outcome.GetError().GetMessage());
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
        Log(Logging::LogLevel::Error,
            "Failed to detach Certificate from Thing",
            thingName,
            outcome.GetError().GetMessage());
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
        Log(Logging::LogLevel::Error, "Failed to delete Certificate", certificateId, outcome.GetError().GetMessage());
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
    for (const string &thingGroup : thingGroupsToCleanup)
    {
        DeleteThingGroup(thingGroup);
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
        Log(Logging::LogLevel::Error, "JobExecution for Job", jobId, "still IN_PROGRESS after max retries");
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
            Log(Logging::LogLevel::Error, "Failed to describe Tunnel", tunnelId, outcome.GetError().GetMessage());
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
        Log(Logging::LogLevel::Error, "Failed to open Tunnel to Thing", thingName, outcome.GetError().GetMessage());
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

std::string IntegrationTestResourceHandler::GetTargetArn(const std::string &thingName)
{
    std::string arn;

    DescribeThingRequest request;
    request.SetThingName(thingName);

    DescribeThingOutcome outcome = iotClient.DescribeThing(request);

    if (!outcome.IsSuccess())
    {
        Log(Logging::LogLevel::Error, "Failed to describe Thing", thingName, outcome.GetError().GetMessage());
    }
    else
    {
        arn = outcome.GetResult().GetThingArn();
    }
    return arn;
}
std::string IntegrationTestResourceHandler::GetResourceId(const std::string &resource)
{
    return resource.substr(resource.find('/'));
}
vector<ActiveViolation> IntegrationTestResourceHandler::GetViolations(const std::string &profileName)
{
    vector<ActiveViolation> violations;

    ListActiveViolationsRequest request;
    request.SetSecurityProfileName(profileName);

    ListActiveViolationsOutcome outcome = iotClient.ListActiveViolations(request);

    if (!outcome.IsSuccess())
    {
        Log(Logging::LogLevel::Error,
            "Failed to list Active Violations for",
            profileName,
            outcome.GetError().GetMessage());
    }
    else
    {
        violations = outcome.GetResult().GetActiveViolations();
    }
    if (violations.empty())
    {
        Log(Logging::LogLevel::Info, "Found no violations for Security Profile", profileName);
    }
    return violations;
}
void IntegrationTestResourceHandler::CreateAndAttachSecurityProfile(
    const string &profileName,
    const string &thingGroupName,
    const vector<std::string> &metrics)
{
    MetricValue metricValue;
    metricValue.SetCount(1L);

    BehaviorCriteria criteria;
    criteria.SetComparisonOperator(ComparisonOperator::less_than);
    criteria.SetDurationSeconds(300);
    criteria.SetValue(metricValue);
    criteria.SetConsecutiveDatapointsToAlarm(1);
    criteria.SetConsecutiveDatapointsToClear(10);

    CreateSecurityProfileRequest request;
    request.SetSecurityProfileName(profileName);

    for (const string &metric : metrics)
    {
        Behavior behavior;
        behavior.SetCriteria(criteria);
        behavior.SetMetric(metric);
        behavior.SetName(metric);

        request.AddBehaviors(behavior);
    }

    CreateSecurityProfileOutcome outcome = iotClient.CreateSecurityProfile(request);

    Log(Logging::LogLevel::Info, "Creating Security Profile", profileName);

    if (!outcome.IsSuccess())
    {
        Log(Logging::LogLevel::Error,
            "Failed to create Security Profile",
            profileName,
            outcome.GetError().GetMessage());
    }

    AttachSecurityProfile(profileName, thingGroupName);
}

void IntegrationTestResourceHandler::AttachSecurityProfile(
    const std::string &profileName,
    const std::string &thingGroupName)
{
    string thingGroupArn = targetArn.substr(0, targetArn.find_last_of(':') + 1) + "thinggroup/" + thingGroupName;

    AttachSecurityProfileRequest request;
    request.SetSecurityProfileName(profileName);
    request.SetSecurityProfileTargetArn(thingGroupArn);

    AttachSecurityProfileOutcome outcome = iotClient.AttachSecurityProfile(request);

    if (!outcome.IsSuccess())
    {
        Log(Logging::LogLevel::Error,
            "Failed to attach Security Profile",
            profileName,
            outcome.GetError().GetMessage());
    }
}

void IntegrationTestResourceHandler::DeleteSecurityProfile(const string &profileName)
{
    DeleteSecurityProfileRequest request;
    request.SetSecurityProfileName(profileName);

    DeleteSecurityProfileOutcome outcome = iotClient.DeleteSecurityProfile(request);

    if (!outcome.IsSuccess())
    {
        Log(Logging::LogLevel::Error,
            "Failed to delete Security Profile",
            profileName,
            outcome.GetError().GetMessage());
    }
}
void IntegrationTestResourceHandler::DeleteThingGroup(const string &thingGroupName)
{
    DeleteThingGroupRequest request;
    request.SetThingGroupName(thingGroupName);

    DeleteThingGroupOutcome outcome = iotClient.DeleteThingGroup(request);

    if (!outcome.IsSuccess())
    {
        Log(Logging::LogLevel::Error, "Failed to Delete Thing Group", thingGroupName, outcome.GetError().GetMessage());
    }
}
void IntegrationTestResourceHandler::CreateThingGroup(const string &thingGroupName)
{
    CreateThingGroupRequest request;
    request.SetThingGroupName(thingGroupName);

    CreateThingGroupOutcome outcome = iotClient.CreateThingGroup(request);

    if (!outcome.IsSuccess())
    {
        Log(Logging::LogLevel::Error, "Failed to create Thing Group", thingGroupName, outcome.GetError().GetMessage());
    }
    else
    {
        thingGroupsToCleanup.push_back(thingGroupName);
    }
}
void IntegrationTestResourceHandler::AddThingToThingGroup(const string &thingGroupName, const string &thingName)
{
    AddThingToThingGroupRequest request;
    request.SetThingGroupName(thingGroupName);
    request.SetThingName(thingName);

    AddThingToThingGroupOutcome outcome = iotClient.AddThingToThingGroup(request);

    if (!outcome.IsSuccess())
    {
        Log(Logging::LogLevel::Error, "Failed to add Thing to Thing Group", thingName, outcome.GetError().GetMessage());
    }
}
void IntegrationTestResourceHandler::Log(
    Aws::Utils::Logging::LogLevel logLevel,
    const std::string &logMessage,
    const std::string &resource,
    const std::string &errorMessage)
{
    Aws::OStringStream ss;
    ss << logMessage << string{" "} << resource;
    if (logLevel == Logging::LogLevel::Error)
    {
        ss << string{" "} << errorMessage;
    }

    logger->LogStream(logLevel, TAG, ss);
}
