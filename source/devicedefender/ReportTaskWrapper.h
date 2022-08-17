// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef AWS_IOT_DEVICE_CLIENT_REPORTTASKWRAPPER_H
#define AWS_IOT_DEVICE_CLIENT_REPORTTASKWRAPPER_H

#include <aws/iotdevicedefender/DeviceDefender.h>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace DeviceDefender
            {
                /**
                 * \brief Interface for ReportTaskWrapper
                 *
                 * This interface is to allow the injection of Mocks for the purposes of testing Device Defender
                 * Feature.
                 */
                class AbstractReportTask
                {
                  public:
                    virtual ~AbstractReportTask() = default;
                    virtual int StartTask() = 0;
                    virtual void StopTask() = 0;
                };
                /**
                 * \brief A wrapper class for Aws::Iotdevicedefenderv1::ReportTask
                 *
                 * This wrapper class is necessary to facilitate testing for Device Defender Feature
                 * The ReportClass class in the SDK is final and therefore cannot be mocked.
                 */
                class ReportTaskWrapper : public AbstractReportTask
                {
                  public:
                    ReportTaskWrapper() = default;
                    explicit ReportTaskWrapper(std::shared_ptr<Aws::Iotdevicedefenderv1::ReportTask> task);
                    virtual ~ReportTaskWrapper() = default;
                    virtual int StartTask() override;
                    virtual void StopTask() override;

                  private:
                    std::shared_ptr<Aws::Iotdevicedefenderv1::ReportTask> task;
                };
            } // namespace DeviceDefender
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // AWS_IOT_DEVICE_CLIENT_REPORTTASKWRAPPER_H
