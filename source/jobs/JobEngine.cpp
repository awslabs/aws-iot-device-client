// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "JobEngine.h"
#include "../config/Config.h"
#include "../logging/LoggerFactory.h"

#include <array>
#include <cstring>
#include <iterator>
#include <memory>
#include <thread>

#include <sys/wait.h>
#include <unistd.h>

#define PIPE_READ 0
#define PIPE_WRITE 1
#define CMD_FAILURE 1

using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::Jobs;
using namespace Aws::Iot::DeviceClient::Logging;
using namespace std;

void JobEngine::processCmdOutput(int fd, bool isStdErr, int childPID)
{
    array<char, 1024> buffer;
    unique_ptr<FILE, decltype(&fclose)> pipe(fdopen(fd, "r"), &fclose);
    if (NULL == pipe.get())
    {
        LOGM_ERROR(
            TAG, "Failed to open pipe to %s for job, errno: %s", isStdErr ? "STDERR" : "STDOUT", strerror(errno));
        return;
    }

    string pidString = std::to_string(childPID);
    char const *logTag = pidString.c_str();

    size_t lineCount = 0;
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        if (lineCount > MAX_LOG_LINES)
        {
            string limitMessage = Util::FormatMessage(
                "*** The specified job has exceeded the maximum output limit for %s, no further output will be written "
                "from this file descriptor for this job ***",
                isStdErr ? "STDERR" : "STDOUT");
            if (isStdErr)
            {
                LOG_ERROR(TAG, limitMessage.c_str());
            }
            else
            {
                LOG_DEBUG(TAG, limitMessage.c_str());
            }
            return;
        }
        string childOutput;
        childOutput += buffer.data();
        childOutput = Util::Sanitize(childOutput);
        if (isStdErr)
        {
            stderrstream.addString(childOutput);
            if ('\n' == childOutput[childOutput.size() - 1])
            {
                childOutput.pop_back();
            }
            LOG_ERROR(logTag, childOutput.c_str());
            this->errors.fetch_add(1);
        }
        else
        {
            stdoutstream.addString(childOutput);
            if ('\n' == childOutput[childOutput.size() - 1])
            {
                childOutput.pop_back();
            }
            LOG_DEBUG(logTag, childOutput.c_str());
        }
        lineCount++;
    }
}

string JobEngine::buildCommand(Optional<string> path, const Optional<std::string> &handler, const std::string &jobHandlerDir)
    const
{
    ostringstream commandStream;
    bool operationOwnedByDeviceClient = false;
    if (path.has_value() && DEFAULT_PATH_KEYWORD == path.value())
    {
        LOGM_DEBUG(
            TAG, "Using DC default command path {%s} for command execution", Util::Sanitize(jobHandlerDir).c_str());
        operationOwnedByDeviceClient = true;
        commandStream << jobHandlerDir;
        if (jobHandlerDir.back() != Config::PATH_DIRECTORY_SEPARATOR)
        {
            commandStream << Config::PATH_DIRECTORY_SEPARATOR;
        }
    }
    else if (path.has_value() && !path.value().empty())
    {
        LOGM_DEBUG(
            TAG,
            "Using path {%s} supplied by job document for command execution",
            Util::Sanitize(path.value()).c_str());
        commandStream << path.value();
        if (path.value().back() != Config::PATH_DIRECTORY_SEPARATOR)
        {
            commandStream << Config::PATH_DIRECTORY_SEPARATOR;
        }
    }
    else
    {
        LOG_DEBUG(TAG, "Assuming executable is in PATH");
    }

    commandStream << handler->c_str();

    if (operationOwnedByDeviceClient)
    {
        const int actualPermissions = Util::FileUtils::GetFilePermissions(commandStream.str());
        if (Permissions::JOB_HANDLER != actualPermissions)
        {
            string message = Util::FormatMessage(
                "Unacceptable permissions found for job handler %s, permissions should be %d but found %d",
                Util::Sanitize(commandStream.str()).c_str(),
                Permissions::JOB_HANDLER,
                actualPermissions);
            LOG_ERROR(TAG, message.c_str());
            throw std::runtime_error(message);
        }
    }
    return commandStream.str();
}

void JobEngine::exec_action(PlainJobDocument::JobAction action, const std::string &jobHandlerDir, int &executionStatus)
{
    string command;
    if (action.type == PlainJobDocument::ACTION_TYPE_RUN_HANDLER)
    {
        // build command for runHandler type
        try
        {
            command = buildCommand(action.input.path, action.input.handler, jobHandlerDir);
        }
        catch (exception &e)
        {
            if (!action.ignoreStepFailure.value())
            {
                executionStatus = 1;
            }
            return;
        }
    }
    else if(action.type == PlainJobDocument::ACTION_TYPE_RUN_COMMAND)
    {
        //build commands for runCommand type
        string first_string = action.input.commands->front();
        command = first_string.substr(0, first_string.find(' '));
    }
    else
    {
        LOG_ERROR(TAG, "Job Document received with invalid action type.");
        executionStatus = 1;
        return;
    }

    ostringstream argsStringForLogging;
    if (action.input.args.has_value())
    {
        for (const auto &eachArgument : action.input.args.value())
        {
            argsStringForLogging << eachArgument << " ";
        }
    }
    else if (action.input.commands.has_value())
    {
        //print out commands on console
        for (const auto &eachCommand : action.input.commands.value())
        {
            argsStringForLogging << eachCommand << " ";
        }
    }
    else
    {
        LOG_INFO(
            TAG, "Did not find any arguments in the incoming job document. Value should be a JSON array of arguments");
    }

    LOGM_INFO(
        TAG,
        "About to execute: %s %s %s",
        Util::Sanitize(command).c_str(),
        Util::Sanitize(action.runAsUser->c_str()).c_str(),
        Util::Sanitize(argsStringForLogging.str()).c_str());

    int actionExecutionStatus;
    if(action.type == RUN_HANDLER_TYPE)
    {
        actionExecutionStatus = exec_cmd(command, action);
    }
    else
    {
        actionExecutionStatus = exec_shellCommand(command, action);
    }

    if (!action.ignoreStepFailure.value())
    {
        if (action.allowStdErr.has_value())
        {
            if (!actionExecutionStatus && this->hasErrors() >= action.allowStdErr.value())
            {
                executionStatus = actionExecutionStatus;
                return;
            }
        }
        else
        {
            executionStatus = actionExecutionStatus;
            return;
        }
    }
}

int JobEngine::exec_steps(PlainJobDocument jobDocument, const std::string &jobHandlerDir)
{
    int executionStatus = 0;
    for (const auto &action : jobDocument.steps)
    {
        LOGM_INFO(TAG, "About to execute step with name: %s", Util::Sanitize(action.name).c_str());
        exec_action(action, jobHandlerDir, executionStatus);
        if (this->hasErrors())
        {
            LOGM_WARN(
                TAG, "While executing action %s, JobEngine reported receiving errors from STDERR", action.name.c_str());
        }
        if (executionStatus != 0)
        {
            return executionStatus;
        }
    }

    if (jobDocument.finalStep.has_value())
    {
        exec_action(jobDocument.finalStep.value(), jobHandlerDir, executionStatus);
        LOGM_INFO(
            TAG, "About to execute step with name: %s", Util::Sanitize(jobDocument.finalStep->name.c_str()).c_str());
    }
    return executionStatus;
}

int JobEngine::exec_cmd(const string &operation, PlainJobDocument::JobAction action)
{
    // Establish some file descriptors which we'll use to redirect stdout and
    // stderr from the child process back into our logger
    int stdout[] = {0, 0};
    int stderr[] = {0, 0};

    if (pipe(stdout) < 0)
    {
        LOG_ERROR(TAG, "failed allocating pipe for child STDOUT redirect");
        return CMD_FAILURE;
    }

    if (pipe(stderr) < 0)
    {
        close(stdout[PIPE_READ]);
        close(stdout[PIPE_WRITE]);
        LOG_ERROR(TAG, "failed allocating pipe for child STDERR redirect");
        return CMD_FAILURE;
    }

    /**
     * \brief Create char array argv[] storing arguments to pass to execvp() function.
     * argv[0] executable path
     * argv[1] Linux user name
     * argv[2:] arguments required for executing the executable file..
     */
    size_t argSize = 0;
    if (action.input.args.has_value())
    {
        argSize = action.input.args->size();
    }
    std::unique_ptr<const char *[]> argv(new const char *[argSize + 3]);
    argv[0] = operation.c_str();
    argv[1] = action.runAsUser->c_str();
    argv[argSize + 2] = nullptr;
    for (size_t i = 0; i < argSize; i++)
    {
        argv[i + 2] = action.input.args->at(i).c_str();
    }

    int execResult;
    int returnCode;
    int pid = vfork();
    if (pid < 0)
    {
        LOGM_ERROR(TAG, "Failed to create child process, fork returned %d", pid);
        return CMD_FAILURE;
    }
    else if (pid == 0)
    {
        // Child process
        LOG_DEBUG(TAG, "Child process now running");

        // redirect stdout
        if (dup2(stdout[PIPE_WRITE], STDOUT_FILENO) == -1)
        {
            LOGM_WARN(TAG, "Failed to duplicate STDOUT pipe, errno {%d}, stdout will likely be unavailable", errno);
        }

        // redirect stderr
        if (dup2(stderr[PIPE_WRITE], STDERR_FILENO) == -1)
        {
            LOGM_WARN(TAG, "Failed to duplicate STDERR pipe, errno {%d}, stderr will likely be unavailable", errno);
        }

        // all these are for use by parent only
        // TODO we need to make sure ALL file handles get closed, including those within the MQTTConnectionManager
        close(stdout[PIPE_READ]);
        close(stdout[PIPE_WRITE]);
        close(stderr[PIPE_READ]);
        close(stderr[PIPE_WRITE]);

        LOG_DEBUG(TAG, "Child process about to call execvp");

        if (execvp(operation.c_str(), const_cast<char *const *>(argv.get())) == -1)
        {
            LOGM_DEBUG(TAG, "Failed to invoke execvp system call to execute action step: %s ", strerror(errno));
        }
        // If the exec fails we need to exit the child process
        _exit(1);
    }
    else
    {
        // parent process
        LOGM_DEBUG(TAG, "Parent process now running, child PID is %d", pid);
        // close unused file descriptors
        close(stdout[PIPE_WRITE]);
        close(stderr[PIPE_WRITE]);

        // Set up some threads to process the output from the child process
        thread stdOutProcessor(&JobEngine::processCmdOutput, this, stdout[PIPE_READ], false, pid);
        stdOutProcessor.join();
        thread stdErrProcessor(&JobEngine::processCmdOutput, this, stderr[PIPE_READ], true, pid);
        stdErrProcessor.join();

        do
        {
            // TODO: do not wait for infinite time for child process to complete
            int waitReturn = waitpid(pid, &execResult, 0);
            if (waitReturn == -1)
            {
                LOG_WARN(TAG, "Failed to wait for child process");
            }

            LOGM_DEBUG(TAG, "JobEngine finished waiting for child process, returning %d", execResult);
            returnCode = execResult;
        } while (!WIFEXITED(execResult) && !WIFSIGNALED(execResult));
    }
    return returnCode;
}

int JobEngine::exec_shellCommand(const std::string &command, PlainJobDocument::JobAction action)
{
    //first to run command "id $user" and "command -v sudo" to verify user and sudo
    std::unique_ptr<const char *[]> argv1(new const char *[3]);
    argv1[0] = "id";
    argv1[1] = action.runAsUser->c_str();
    argv1[2] = nullptr;



    int status1, status2;
    int execStatus1, execStatus2;
    int pid1 = vfork();
    if (pid1 < 0)
    {
        LOGM_ERROR(TAG, "Failed to create child process, fork returned %d", pid1);
        return CMD_FAILURE;
    }
    else if (pid1 == 0)
    {
        LOG_DEBUG(TAG, "Child process now running, verifying user");

        auto rc = execvp(argv1[0], const_cast<char *const *>(argv1.get()));
        if(rc == -1)
        {
            auto err = errno;
            LOGM_DEBUG(TAG, "Failed to invoke execvp system call to execute action step: %s (%d)", strerror(err), err);
            _exit(rc);
            //
        }
        LOGM_DEBUG(TAG, "execvp() %d", rc);
        _exit(0);
    }
    else
    {
        do
        {
            // TODO: do not wait for infinite time for child process to complete
            int waitReturn = waitpid(pid1, &status1, 0);
            if (waitReturn == -1)
            {
                LOG_WARN(TAG, "Failed to wait for child process");
            }

            LOGM_DEBUG(TAG, "JobEngine finished waiting for child process, returning %d", status1);
            execStatus1 = status1;
        } while (!WIFEXITED(status1) && !WIFSIGNALED(status1));

    }

    std::unique_ptr<const char *[]> argv2(new const char *[4]);
    argv2[0] = "command";
    argv2[1] = "-v";
    argv2[2] = "sudo";
    argv2[3] = nullptr;

    int pid2 = vfork();
    if (pid2 < 0)
    {
        LOGM_ERROR(TAG, "Failed to create child process, fork returned %d", pid2);
        return CMD_FAILURE;
    }
    else if (pid2 == 0)
    {
        LOG_DEBUG(TAG, "Child process now running, verifying sudo");

        if(execvp(argv2[0], const_cast<char *const *>(argv2.get())) == -1)
        {
            LOGM_DEBUG(TAG, "Failed to invoke execvp system call to execute action step: %s ", strerror(errno));
        }
        _exit(1);
    }
    else
    {
        do
        {
            // TODO: do not wait for infinite time for child process to complete
            int waitReturn = waitpid(pid2, &status2, 0);
            if (waitReturn == -1)
            {
                LOG_WARN(TAG, "Failed to wait for child process");
            }

            LOGM_DEBUG(TAG, "JobEngine finished waiting for child process, returning %d", status2);
            execStatus2 = status2;
        } while (!WIFEXITED(status2) && !WIFSIGNALED(status2));

    }

    int returnCode;

    //if two verifications succeeds, build command using sudo -u $user -n $@ and execute
    if(execStatus1 == -1 || execStatus2 == -1)
    {
        LOG_WARN(TAG, "username or sudo command not found");
        //still execute code?
    }
    else
    {
        // Establish some file descriptors which we'll use to redirect stdout and
        // stderr from the child process back into our logger
        int stdout[] = {0, 0};
        int stderr[] = {0, 0};

        if (pipe(stdout) < 0)
        {
            LOG_ERROR(TAG, "failed allocating pipe for child STDOUT redirect");
            return CMD_FAILURE;
        }

        if (pipe(stderr) < 0)
        {
            close(stdout[PIPE_READ]);
            close(stdout[PIPE_WRITE]);
            LOG_ERROR(TAG, "failed allocating pipe for child STDERR redirect");
            return CMD_FAILURE;
        }
        /**
        * \brief Create char array argv[] storing arguments to pass to execvp() function.
        * argv[0] executable path
        * argv[1] Linux user name
        * argv[2:] arguments required for executing the executable file..
        */
        size_t argSize = 0;
        if (action.input.commands.has_value())
        {
            argSize = action.input.commands->size();
        }
        std::unique_ptr<const char *[]> argv(new const char *[argSize + 5]);
        argv[0] = "sudo";
        argv[1] = "-u";
        argv[2] = action.runAsUser->c_str();
        argv[3] = "-n";
        argv[argSize + 4] = nullptr;
        for (size_t i = 0; i < argSize; i++)
        {
            argv[i + 4] = action.input.args->at(i).c_str();
        }

        int execResult;
        int pid = vfork();
        if (pid < 0)
        {
            LOGM_ERROR(TAG, "Failed to create child process, fork returned %d", pid);
            return CMD_FAILURE;
        }
        else if (pid == 0)
        {
            // Child process
            LOG_DEBUG(TAG, "Child process now running");

            // redirect stdout
            if (dup2(stdout[PIPE_WRITE], STDOUT_FILENO) == -1)
            {
                LOGM_WARN(TAG, "Failed to duplicate STDOUT pipe, errno {%d}, stdout will likely be unavailable", errno);
            }

            // redirect stderr
            if (dup2(stderr[PIPE_WRITE], STDERR_FILENO) == -1)
            {
                LOGM_WARN(TAG, "Failed to duplicate STDERR pipe, errno {%d}, stderr will likely be unavailable", errno);
            }

            // all these are for use by parent only
            // TODO we need to make sure ALL file handles get closed, including those within the MQTTConnectionManager
            close(stdout[PIPE_READ]);
            close(stdout[PIPE_WRITE]);
            close(stderr[PIPE_READ]);
            close(stderr[PIPE_WRITE]);

            LOG_DEBUG(TAG, "Child process about to call execvp");

            if (execvp(argv[0], const_cast<char *const *>(argv.get())) == -1)
            {
                LOGM_DEBUG(TAG, "Failed to invoke execvp system call to execute action step: %s ", strerror(errno));
            }
            // If the exec fails we need to exit the child process
            _exit(1);
        }
        else
        {
            // parent process
            LOGM_DEBUG(TAG, "Parent process now running, child PID is %d", pid);
            // close unused file descriptors
            close(stdout[PIPE_WRITE]);
            close(stderr[PIPE_WRITE]);

            // Set up some threads to process the output from the child process
            thread stdOutProcessor(&JobEngine::processCmdOutput, this, stdout[PIPE_READ], false, pid);
            stdOutProcessor.join();
            thread stdErrProcessor(&JobEngine::processCmdOutput, this, stderr[PIPE_READ], true, pid);
            stdErrProcessor.join();

            do
            {
                // TODO: do not wait for infinite time for child process to complete
                int waitReturn = waitpid(pid, &execResult, 0);
                if (waitReturn == -1)
                {
                    LOG_WARN(TAG, "Failed to wait for child process");
                }

                LOGM_DEBUG(TAG, "JobEngine finished waiting for child process, returning %d", execResult);
                returnCode = execResult;
            } while (!WIFEXITED(execResult) && !WIFSIGNALED(execResult));
        }
    }
    return returnCode;

}

string JobEngine::getReason(int statusCode)
{
    ostringstream reason;
    if (WIFEXITED(statusCode))
    {
        reason << "Job exited with status: " << WEXITSTATUS(statusCode);
    }
    else if (WIFSIGNALED(statusCode))
    {
        reason << "Job killed by signal: " << WTERMSIG(statusCode);
    }
    else if (WIFSTOPPED(statusCode))
    {
        reason << "Job stopped by signal: " << WSTOPSIG(statusCode);
    }
    else
    {
        reason << "Job returned with status: " << statusCode;
    }

    return reason.str();
}
