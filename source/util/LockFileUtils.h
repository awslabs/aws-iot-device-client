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
                 * \brief Utility functions for creating/deleting the
                 */
                class LockFileUtils
                {
                private:
                    static constexpr char TAG[] = "LockFileUtils.cpp";
                    static constexpr char FILE_NAME[] = "/var/run/devicecl.lock";
                    static constexpr char PROCESS_NAME[] = "aws-iot-device-client";

                    /**
                     * \brief
                     * @param pid
                     */
                    static void WriteToLockFile(const std::string &pid);

                public:
                    /**
                     * \brief Creates each of the directories in the provided path if they do not exist
                     * @param path the full path to assess
                     * @return 0 upon success, some other number indicating an error otherwise
                     */
                    static int ProcessLock();

                    /**
                     * \brief Given a path to a file, attempts to extract the parent directory
                     * @param filePath a path to a file
                     * @return 0 upon success, some other number indicating an error otherwise
                     */
                    static int ProcessUnlock();




                };
            } // namespace Util
        } // namespace DeviceClient
    } // namespace Iot
} // namespace Aws

#endif //AWS_IOT_DEVICE_CLIENT_LOCKFILEUTILS_H
