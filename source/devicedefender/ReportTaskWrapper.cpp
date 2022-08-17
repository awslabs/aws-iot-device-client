// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "ReportTaskWrapper.h"

using namespace Aws::Iot;
using namespace std;

DeviceClient::DeviceDefender::ReportTaskWrapper::ReportTaskWrapper(shared_ptr<Iotdevicedefenderv1::ReportTask> task)
    : task(task)
{
}

int DeviceClient::DeviceDefender::ReportTaskWrapper::StartTask()
{
    return task->StartTask();
}
void DeviceClient::DeviceDefender::ReportTaskWrapper::StopTask()
{
    task->StopTask();
}
