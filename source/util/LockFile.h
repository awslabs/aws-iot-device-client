// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef AWS_IOT_DEVICE_CLIENT_LOCKFILE_H
#define AWS_IOT_DEVICE_CLIENT_LOCKFILE_H

#include <string>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Util
            {
                struct LockFile
                {
                  private:
                    static constexpr char TAG[] = "LockFile.cpp";
                    std::string filename;

                  public:
                    explicit LockFile(const std::string &filename);
                    ~LockFile();

                    // Non-copyable.
                    LockFile(const LockFile &) = delete;
                    LockFile &operator=(const LockFile &) = delete;
                };
            } // namespace Util
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // AWS_IOT_DEVICE_CLIENT_LOCKFILE_H
