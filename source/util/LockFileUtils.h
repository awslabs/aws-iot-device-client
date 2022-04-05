// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef AWS_IOT_DEVICE_CLIENT_LOCKFILEUTILS_H
#define AWS_IOT_DEVICE_CLIENT_LOCKFILEUTILS_H

#include <string>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Util
            {
                /**
                 * \brief Utility functions for creating/deleting the lock file
                 */
                class LockFileUtils
                {
                    private:
                        static constexpr char TAG[] = "LockFileUtils.cpp";
                        static constexpr char FILE_NAME[] = "/var/run/devicecl.lock";
                        static constexpr char PROCESS_NAME[] = "aws-iot-device-client";

                        /**
                         * \brief Gives the current process exclusive write access and writes the pid into the lock file
                         * @param pid
                         */
                        static void WriteToLockFile(const std::string &pid);

                    public:
                        /**
                         * \brief Creates a lock file in the default directory if it does not exist, aborts execution if another process is running.
                         * @return 0 upon success
                         */
                        static int ProcessLock();

                        /**
                         * \brief Deletes the lock file when device client shuts down
                         * @return 0 upon success
                         */
                        static int ProcessUnlock();
                };
            } // namespace Util
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // AWS_IOT_DEVICE_CLIENT_LOCKFILEUTILS_H
