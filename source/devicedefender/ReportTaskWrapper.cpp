// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "ReportTaskWrapper.h"

Iot::DeviceClient::DeviceDefender::ReportTaskWrapper::ReportTaskWrapper(shared_ptr<Iotdevicedefenderv1::ReportTask> task)
    : task(task){}

int Iot::DeviceClient::DeviceDefender::ReportTaskWrapper::StartTask()
{
    return task->StartTask();
}
void Iot::DeviceClient::DeviceDefender::ReportTaskWrapper::StopTask()
{
    task->StopTask();
}
