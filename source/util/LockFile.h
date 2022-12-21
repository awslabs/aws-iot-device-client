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
                    static constexpr char FILE_NAME[] = "devicecl.lock";
                    std::string dir;

                  public:
                    /**
                     * \brief Constructor will enforce single process creation by writing a file to a specified
                     * directory. If lockfile already exists, reads the pid from the file and checks if a process is
                     * currently running with the pid. If the running process is device client, throw an exception, else
                     * delete the file and rewrite the current pid.
                     *
                     * @param filedir directory the lockfile will be written to
                     * @param process the executable path passed in by argv[0], usually aws-iot-device-client
                     */
                    LockFile(const std::string &filedir, const std::string &process, const std::string &thingName);
                    /**
                     * This class uses RAII for resource management. Destructor will be called on program exit and the
                     * file will be deleted.
                     */
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
