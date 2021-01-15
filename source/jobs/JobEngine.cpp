// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "JobEngine.h"
#include "../logging/LoggerFactory.h"

#include <array>
#include <cstring>
#include <iterator>
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

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
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
    }
}

int JobEngine::exec_cmd(string action, vector<string> args)
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

    const char **argv = new const char *[args.size() + 2];
    argv[0] = action.c_str();
    argv[args.size() + 1] = nullptr;
    for (size_t i = 0; i < args.size(); i++)
    {
        argv[i + 1] = args.at(i).c_str();
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

        execvp(action.c_str(), const_cast<char *const *>(argv));
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