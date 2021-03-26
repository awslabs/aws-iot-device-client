// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef DEVICE_CLIENT_JOBENGINE_H
#define DEVICE_CLIENT_JOBENGINE_H

#include <atomic>
#include <condition_variable>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "LimitedStreamBuffer.h"

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Jobs
            {
                /**
                 * \brief Manages the execution of a Job
                 *
                 * The JobEngine is fully responsible for executing a given command and its arguments, which may
                 * point to handlers provided as part of the Device Client or to other executables available to
                 * the device. The JobEngine manages all of the setup required to redirect output from the child
                 * process so that it can be analyzed by the Jobs feature and used to determine job success.
                 */
                class JobEngine
                {
                  private:
                    const char *TAG = "JobEngine.cpp";
                    /**
                     * \brief The maximum number of lines that we'll read from STDOUT or STDERR of the child process
                     * before stopping. This prevents against log corruption in the event that the specified
                     * job generates a large volume of output
                     */
                    static constexpr size_t MAX_LOG_LINES = 1000;

                    /**
                     * \brief The number of lines received on STDERR from the child process
                     *
                     * Used to determine whether the job was successful or not, since a script
                     * with multiple commands will return the return code of the final command
                     * and may not be indicative of whether all actions were successful. The incoming
                     * job document may include a property that specifies an acceptable number of
                     * STDERR lines to allow in case some errors are expected.
                     */
                    std::atomic_int errors{0};

                    /**
                     * \brief Partial output from STDOUT of the child process to be used in UpdateJobExecution
                     */
                    Aws::Iot::DeviceClient::Jobs::LimitedStreamBuffer stdoutstream;
                    /**
                     * \brief Partial output from STDERR of the child process to be used in UpdateJobExecution
                     */
                    Aws::Iot::DeviceClient::Jobs::LimitedStreamBuffer stderrstream;

                  public:
                    /**
                     * \brief Used by output processing threads to assess output from the child process
                     *
                     * @param fd the file descriptor of the output to process
                     * @param isStdErr whether the output being processed is from STDERR
                     * @param childPID the process ID of the child process
                     */
                    void processCmdOutput(int fd, bool isStdErr, int childPID);

                    /**
                     * \brief Executes the given command (action) and passes the provided vector of arguments to that
                     * command
                     * @param action the command to execute
                     * @param args the arguments to pass to that command
                     * @return an integer representing the return code of the executed process
                     */
                    int exec_cmd(std::string action, std::vector<std::string> args);
                    /**
                     * \brief Begin the execution of a command with the specified arguments
                     *
                     * @param operation the operation to perform, likely the path to an executable
                     * @param args the arguments to pass to the executable
                     * @return the return code of the child process, or an error code if it cannot be executed
                     */
                    /**
                     * \brief Whether the JobEngine is reporting errors received from the child process
                     *
                     * @return an integer representing the number of lines received on STDERR
                     */
                    int hasErrors() { return errors; }

                    /**
                     * \brief Evaluates the return code of the JobEngine's command execution
                     * @param statusCode the status code returned by the job execution
                     * @return the output of the status code evaluation
                     */
                    std::string getReason(int statusCode);

                    /**
                     * \brief Take the STDOUT received from the child process
                     *
                     * @return a LimitedStreamBuffer taken from the JobEngine
                     */
                    std::string getStdOut() { return stdoutstream.toString(); };

                    /**
                     * \brief Take the STDERR received from the child process
                     *
                     * @return a LimitedStreamBuffer taken from the JobEngine
                     */
                    std::string getStdErr() { return stderrstream.toString(); };
                };
            } // namespace Jobs
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_JOBENGINE_H
