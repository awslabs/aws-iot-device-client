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

#include "../util/FileUtils.h"
#include "JobDocument.h"
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
                     * \brief A keyword that can be specified as the "path" in a job doc to tell the Jobs feature to
                     * use the configured handler directory when looking for an executable matching the specified
                     * operation
                     */
                    const char *DEFAULT_PATH_KEYWORD = "default";

                    const char *RUN_HANDLER_TYPE = "runHandler";
                    const char *RUN_COMMAND_TYPE = "runCommand";

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

                    /**
                     * \brief Builds the command that will be executed
                     * @param path the provided path to the executable
                     * @param handler the name of the handler script or executable file
                     * @return the full executable path.
                     *
                     * If this command is unable to find a given job handler and/or the permissions
                     * for the given job handler are inappropriate, this function will thrown an exception.
                     */
                    std::string buildCommand(
                        Crt::Optional<std::string> path,
                        const std::string &handler,
                        const std::string &jobHandlerDir) const;

                    /**
                     * \brief Executes the argv, consists of command and arguments, using execvp().
                     * This function also opens two pipes to process outputs from child processes.
                     * @param argv the arguments to pass to execvp() to execute
                     * @return an integer representing the return code of the executed process
                     */
                    int exec_cmd(std::unique_ptr<const char *[]> &argv);

                    /**
                     * \brief Executes the argv, consists of command and arguments, using execvp()
                     * This function only returns the exit code of child processes
                     * @param argv the arguments to pass to execvp() to execute
                     * @return an integer representing the return code of the executed process
                     */
                    int exec_process(std::unique_ptr<const char *[]> &argv);

                    /**
                     * \brief Verifies if "sudo" and "$user" exists
                     * @param action the action provided in job document to execute
                     * @return an boolean indicating verification succeeds or fails
                     */
                    bool verifySudoAndUser(PlainJobDocument::JobAction action);

                    /**
                     * \brief Builds argv for "runHandler" type of jobs and makes calls to exec_cmd()
                     * to execute
                     * @param command the command built from handler and path and used by execvp() to execute
                     * @param action the action provided in job document to execute
                     * @return an integer representing the return code of the executed process
                     */
                    int exec_handlerScript(const std::string &command, PlainJobDocument::JobAction action);

                    /**
                     * \brief Builds argv for "runCommand" type of jobs and makes calls to exec_cmd()
                     * to execute
                     * @param action the action provided in job document to execute
                     * @return an boolean indicating verification succeeds or fails
                     */
                    int exec_shellCommand(PlainJobDocument::JobAction action);

                    /**
                     * \brief Executes the given set of steps (actions) in sequence as provided in the job document
                     * @param action the action provided in job document to execute
                     * @param jobHandlerDir the default job handler directory path
                     * @param executionStatus job execution status
                     * @return an integer representing the return code of the executed action
                     */
                    void exec_action(
                        PlainJobDocument::JobAction action,
                        const std::string &jobHandlerDir,
                        int &executionStatus);

                  public:
                    virtual ~JobEngine() = default;
                    /**
                     * \brief Used by output processing threads to assess output from the child process
                     *
                     * @param fd the file descriptor of the output to process
                     * @param isStdErr whether the output being processed is from STDERR
                     * @param childPID the process ID of the child process
                     */
                    virtual void processCmdOutput(int fd, bool isStdErr, int childPID);

                    /**
                     * \brief Executes the given set of steps (actions) in sequence as provided in the job document
                     * @param jobDocument the job document to execute
                     * @param jobHandlerDir the default job handler directory path
                     * @return an integer representing the return code of the executed action
                     */
                    virtual int exec_steps(PlainJobDocument jobDocument, const std::string &jobHandlerDir);
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
                    virtual int hasErrors() { return errors; }

                    /**
                     * \brief Evaluates the return code of the JobEngine's command execution
                     * @param statusCode the status code returned by the job execution
                     * @return the output of the status code evaluation
                     */
                    virtual std::string getReason(int statusCode);

                    /**
                     * \brief Take the STDOUT received from the child process
                     *
                     * @return a LimitedStreamBuffer taken from the JobEngine
                     */
                    virtual std::string getStdOut() { return stdoutstream.toString(); };

                    /**
                     * \brief Take the STDERR received from the child process
                     *
                     * @return a LimitedStreamBuffer taken from the JobEngine
                     */
                    virtual std::string getStdErr() { return stderrstream.toString(); };
                };
            } // namespace Jobs
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws

#endif // DEVICE_CLIENT_JOBENGINE_H
