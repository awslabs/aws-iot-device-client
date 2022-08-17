// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef AWS_IOT_DEVICE_CLIENT_REPORTTASKWRAPPER_H
#define AWS_IOT_DEVICE_CLIENT_REPORTTASKWRAPPER_H

#include <aws/iotdevicedefender/DeviceDefender.h>

using namespace std;
using namespace Aws;

namespace Aws {
    namespace Iot {
        namespace DeviceClient {
            namespace DeviceDefender {

                class AbstractReportTask
                {
                  public:
                    virtual ~AbstractReportTask() = default;
                    virtual int StartTask() = 0;
                    virtual void StopTask() = 0;
                };
                class ReportTaskWrapper : public AbstractReportTask
                {
                  public:
                    ReportTaskWrapper() = default;
                    explicit ReportTaskWrapper(shared_ptr<Aws::Iotdevicedefenderv1::ReportTask> task);
                    virtual ~ReportTaskWrapper() = default;
                    virtual int StartTask();
                    virtual void StopTask();

                  private:
                    std::shared_ptr<Aws::Iotdevicedefenderv1::ReportTask> task;
                };
            }
        }
    }
}



#endif // AWS_IOT_DEVICE_CLIENT_REPORTTASKWRAPPER_H
