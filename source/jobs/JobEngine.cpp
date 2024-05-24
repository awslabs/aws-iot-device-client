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

#ifdef _WIN32
#undef FormatMessage
#ifndef close
#define close _close
#endif /* close */
#endif

constexpr int PIPE_READ = 0;
constexpr int PIPE_WRITE = 1;
constexpr int CMD_FAILURE = 1;

using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::Jobs;
using namespace Aws::Iot::DeviceClient::Logging;
using namespace Aws::Iot::DeviceClient::Util;
using namespace std;

#ifndef _WIN32
void JobEngine::processCmdOutput(int fd, bool isStdErr, int childPID)
{
    array<char, 1024> buffer;
    unique_ptr<FILE, decltype(&fclose)> pipePtr(fdopen(fd, "r"), &fclose);
    if (nullptr == pipePtr.get())
    {
        LOGM_ERROR(
            TAG, "Failed to open pipe to %s for job, errno: %s", isStdErr ? "STDERR" : "STDOUT", strerror(errno));
        return;
    }

    string pidString = std::to_string(childPID);
    char const *logTag = pidString.c_str();

    size_t lineCount = 0;
#ifndef _WIN32    
    while (fgets(buffer.data(), buffer.size(), pipePtr.get()) != nullptr)
#else
    while (fgets(buffer.data(), 1024, pipePtr.get()) != nullptr)
#endif
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
#else
void JobEngine::processCmdOutput(HANDLE hPipe, bool isStdErr, int childPID) {
    DWORD bytesRead;
    //CHAR buffer[4096];
    array<char, 4096> buffer;
    BOOL success;

    string pidString = std::to_string(childPID);
    char const *logTag = pidString.c_str();

    string line;
    while (true) {
        success = ReadFile(hPipe, buffer.data(), 4096 - 1, &bytesRead, NULL);
        if (!success || bytesRead == 0) break;
        buffer[bytesRead] = '\0';

        // Split output into lines
        char* p = buffer.data();        
        for (DWORD i = 0; i < bytesRead; i++) {
            if (*p == '\n' || *p == '\0' || *p == '\r') {
                if (!line.empty()) {
                    string childOutput;
                    //if (*p != '\n') *p = '\n'; // Make sure the line end is there
                    childOutput += line + '\n';
                    line.clear();
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
                if (*p == '\0') 
                    break;
            } else {
                line.push_back(*p);
            }
            p++;
        }
    }
}
#endif

string JobEngine::buildCommand(Crt::Optional<string> path, const std::string &handler, const std::string &jobHandlerDir)
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

    commandStream << handler;

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
            command = buildCommand(action.handlerInput->path, action.handlerInput->handler, jobHandlerDir);
        }
        catch (exception &/*e*/)
        {
            if (!action.ignoreStepFailure.value())
            {
                executionStatus = 1;
            }
            return;
        }
    }
    else if (action.type == PlainJobDocument::ACTION_TYPE_RUN_COMMAND)
    {
        // build commands for runCommand type
        command = action.commandInput->command.front();
    }
    else
    {
        LOG_ERROR(TAG, "Job Document received with invalid action type.");
        executionStatus = 1;
        return;
    }

    ostringstream argsStringForLogging;
    if (action.type == RUN_HANDLER_TYPE && action.handlerInput->args.has_value())
    {
        // build logstream for runHandler to print out on console
        for (const auto &eachArgument : action.handlerInput->args.value())
        {
            argsStringForLogging << eachArgument << " ";
        }
    }
    else if (action.type == RUN_COMMAND_TYPE)
    {
        // build logstream for runCommand to print out on console
        for (size_t i = 1; i < action.commandInput->command.size(); i++)
        {
            argsStringForLogging << action.commandInput->command.at(i) << " ";
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

    int actionExecutionStatus = 0;
    if (action.type == RUN_HANDLER_TYPE)
    {
        actionExecutionStatus = exec_handlerScript(command, action);
    }
    else if (action.type == RUN_COMMAND_TYPE)
    {
        actionExecutionStatus = exec_shellCommand(action);
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

#ifndef _WIN32
int JobEngine::exec_cmd(std::unique_ptr<const char *[]> &argv)
{
    // Establish some file descriptors which we'll use to redirect stdout and
    // stderr from the child process back into our logger
    int stdout_fd[] = {0, 0};
    int stderr_fd[] = {0, 0};

    if (pipe(stdout_fd) < 0)
    {
        LOG_ERROR(TAG, "failed allocating pipe for child STDOUT redirect");
        return CMD_FAILURE;
    }

    if (pipe(stderr_fd) < 0)
    {
        close(stdout_fd[PIPE_READ]);
        close(stdout_fd[PIPE_WRITE]);
        LOG_ERROR(TAG, "failed allocating pipe for child STDERR redirect");
        return CMD_FAILURE;
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

        // redirect stdout_fd
        if (dup2(stdout_fd[PIPE_WRITE], STDOUT_FILENO) == -1)
        {
            LOGM_WARN(TAG, "Failed to duplicate STDOUT pipe, errno {%d}, stdout will likely be unavailable", errno);
        }

        // redirect stderr
        if (dup2(stderr_fd[PIPE_WRITE], STDERR_FILENO) == -1)
        {
            LOGM_WARN(TAG, "Failed to duplicate STDERR pipe, errno {%d}, stderr will likely be unavailable", errno);
        }

        // all these are for use by parent only
        // TODO we need to make sure ALL file handles get closed, including those within the MQTTConnectionManager
        close(stdout_fd[PIPE_READ]);
        close(stdout_fd[PIPE_WRITE]);
        close(stderr_fd[PIPE_READ]);
        close(stderr_fd[PIPE_WRITE]);

        LOG_DEBUG(TAG, "Child process about to call execvp");

        auto rc = execvp(argv[0], const_cast<char *const *>(argv.get()));
        if (rc == -1)
        {
            auto err = errno;
            LOGM_ERROR(TAG, "Failed to invoke execvp system call to execute action step: %s (%d)", strerror(err), err);
            _exit(rc);
        }
        _exit(0);
    }
    else
    {
        // parent process
        LOGM_DEBUG(TAG, "Parent process now running, child PID is %d", pid);
        // close unused file descriptors
        close(stdout_fd[PIPE_WRITE]);
        close(stderr_fd[PIPE_WRITE]);

        // Set up some threads to process the output from the child process
        thread stdOutProcessor(&JobEngine::processCmdOutput, this, stdout_fd[PIPE_READ], false, pid);
        stdOutProcessor.join();
        thread stdErrProcessor(&JobEngine::processCmdOutput, this, stderr_fd[PIPE_READ], true, pid);
        stdErrProcessor.join();

        do
        {
            // TODO: do not wait for infinite time for child process to complete
            int waitReturn = waitpid(pid, &execResult, 0);
            if (waitReturn == -1)
            {
                LOGM_WARN(TAG, "Failed to wait for child process: %d", pid);
            }

            returnCode = WEXITSTATUS(execResult);
            LOGM_DEBUG(TAG, "JobEngine finished waiting for child process, returning %d", returnCode);
        } while (!WIFEXITED(execResult) && !WIFSIGNALED(execResult));
    }
    return returnCode;
}
#else
int JobEngine::exec_cmd(std::unique_ptr<const char *[]> &argv)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;
    HANDLE stdoutRead, stdoutWrite;
    HANDLE stderrRead, stderrWrite;

    // Set up security attributes to allow handle inheritance
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    // Create pipes for stdout and stderr
    if (!CreatePipe(&stdoutRead, &stdoutWrite, &sa, 0)) {
        LOGM_ERROR(TAG, "Failed allocating pipe for child STDOUT redirect: %d", GetLastError());
        if (stdoutRead)
            CloseHandle(stdoutRead);
        if (stdoutWrite)
            CloseHandle(stdoutWrite);

        return CMD_FAILURE;
    }
    if (!CreatePipe(&stderrRead, &stderrWrite, &sa, 0)) {
        LOGM_ERROR(TAG, "Failed allocating pipe for child STDERR redirect: %d", GetLastError());
        if (stdoutRead)
            CloseHandle(stdoutRead);
        if (stdoutWrite)
            CloseHandle(stdoutWrite);
        if (stderrRead)
            CloseHandle(stderrRead);
        if (stderrWrite)
            CloseHandle(stderrWrite);

        return CMD_FAILURE;
    }

    // Ensure the read handles to the pipes are not inherited
    SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(stderrRead, HANDLE_FLAG_INHERIT, 0);

    // Construct the command line string from argv[]
    std::string commandLine;
    for (int i = 0; argv[i] != nullptr; ++i) {
        commandLine += argv[i];
        if (argv[i + 1] != nullptr) { // Add space after each argument except the last one
            commandLine += ' ';
        }
    }

    // Set up members of the STARTUPINFO structure
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = stdoutWrite;
    si.hStdError = stderrWrite;

    // Create the child process
    string strEchoOff = "cmd.exe /Q /C \"" + commandLine + "\"";
    if (!CreateProcess(
        NULL,                  // No module name (use command line)
        const_cast<LPSTR>(strEchoOff.c_str()), // Command line
        NULL,                  // Process handle not inheritable
        NULL,                  // Thread handle not inheritable
        TRUE,                  // Set handle inheritance to TRUE
        CREATE_SUSPENDED | CREATE_NO_WINDOW,    // No creation flags
        NULL,                  // Use parent's environment block
        NULL,                  // Use parent's starting directory 
        &si,                   // Pointer to STARTUPINFO structure
        &pi))                  // Pointer to PROCESS_INFORMATION structure
    {
        DWORD nError = GetLastError();
        LOGM_ERROR(TAG, "Failed to create child process, fork returned %d", nError);

        if (stdoutRead)
            CloseHandle(stdoutRead);
        if (stdoutWrite)
            CloseHandle(stdoutWrite);
        if (stderrRead)
            CloseHandle(stderrRead);
        if (stderrWrite)
            CloseHandle(stderrWrite);

        return CMD_FAILURE;
    }

    // Close handles to the child process's stdin and stdout
    CloseHandle(stdoutWrite);
    CloseHandle(stderrWrite);

    // Create threads to read from the stdout and stderr pipes
    std::thread stdoutThread(&JobEngine::processCmdOutput, this, stdoutRead, FALSE, pi.dwProcessId);
    std::thread stderrThread(&JobEngine::processCmdOutput, this, stderrRead, TRUE, pi.dwProcessId);

    // Resume the child process
    ResumeThread(pi.hThread);

    // Wait for the child process to complete
    DWORD returnCode = WaitForSingleObject(pi.hProcess, INFINITE);
    LOGM_DEBUG(TAG, "JobEngine finished waiting for child process, returning %d", returnCode);

    // Wait for the threads to finish
    stdoutThread.join();
    stderrThread.join();

    // Get the exit code of the child process
    DWORD exitCode;
    if (!GetExitCodeProcess(pi.hProcess, &exitCode) || !stderrstream.toString().empty()) {
        LOGM_ERROR(TAG, "Could not get child process exit code: %d", GetLastError());
        exitCode = CMD_FAILURE;
    }

    // Clean up
    CloseHandle(stdoutRead);
    CloseHandle(stderrRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode;
}
#endif

#ifndef _WIN32
int JobEngine::exec_process(std::unique_ptr<const char *[]> &argv)
{
    int status = 0;
    int execStatus = 0;
    int pid = vfork();

    if (pid < 0)
    {
        auto err = errno;
        LOGM_ERROR(TAG, "Failed to create child process, fork returned: %s (%d)", strerror(err), err);
        return CMD_FAILURE;
    }
    else if (pid == 0)
    {
        LOG_DEBUG(TAG, "Child process now running.");

        auto rc = execvp(argv[0], const_cast<char *const *>(argv.get()));
        if (rc == -1)
        {
            auto err = errno;
            LOGM_ERROR(TAG, "Failed to invoke execvp system call to execute action step: %s (%d)", strerror(err), err);
            _exit(rc);
        }
        _exit(0);
    }
    else
    {
        LOGM_DEBUG(TAG, "Parent process now running, child PID is %d", pid);
        do
        {
            // TODO: do not wait for infinite time for child process to complete
            int waitReturn = waitpid(pid, &status, 0);
            if (waitReturn == -1)
            {
                LOGM_WARN(TAG, "Failed to wait for child process: %d", pid);
            }
            execStatus = WEXITSTATUS(status);
            LOGM_DEBUG(TAG, "JobEngine finished waiting for child process, returning %d", execStatus);

        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return execStatus;
}
#else
int JobEngine::exec_process(std::unique_ptr<const char *[]> &argv)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;
    HANDLE stdoutRead, stdoutWrite;
    HANDLE stderrRead, stderrWrite;

    // Set up security attributes to allow handle inheritance
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    // Create pipes for stdout and stderr
    if (!CreatePipe(&stdoutRead, &stdoutWrite, &sa, 0)) {
        LOGM_ERROR(TAG, "Failed allocating pipe for child STDOUT redirect: %d", GetLastError());
        return CMD_FAILURE;
    }
    if (!CreatePipe(&stderrRead, &stderrWrite, &sa, 0)) {
        LOGM_ERROR(TAG, "Failed allocating pipe for child STDERR redirect: %d", GetLastError());
        return CMD_FAILURE;
    }

    // Ensure the read handles to the pipes are not inherited
    SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(stderrRead, HANDLE_FLAG_INHERIT, 0);

    // Construct the command line string from argv[]
    std::string commandLine;
    for (int i = 0; argv[i] != nullptr; ++i) {
        commandLine += argv[i];
        if (argv[i + 1] != nullptr) { // Add space after each argument except the last one
            commandLine += ' ';
        }
    }

    // Set up members of the STARTUPINFO structure
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = stdoutWrite;
    si.hStdError = stderrWrite;

    // Create the child process
    string strEchoOff = "cmd.exe /Q /C \"" + commandLine + "\"";
    if (!CreateProcess(
        NULL,                  // No module name (use command line)
        const_cast<LPSTR>(strEchoOff.c_str()), // Command line
        NULL,                  // Process handle not inheritable
        NULL,                  // Thread handle not inheritable
        TRUE,                  // Set handle inheritance to TRUE
        CREATE_SUSPENDED | CREATE_NO_WINDOW,    // No creation flags
        NULL,                  // Use parent's environment block
        NULL,                  // Use parent's starting directory 
        &si,                   // Pointer to STARTUPINFO structure
        &pi))                  // Pointer to PROCESS_INFORMATION structure
    {
        DWORD nError = GetLastError();
        LOGM_ERROR(TAG, "Failed to create child process, fork returned %d", nError);
        return CMD_FAILURE;
    }

    // Close handles to the child process's stdin and stdout
    CloseHandle(stdoutWrite);
    CloseHandle(stderrWrite);

    // Resume the child process
    ResumeThread(pi.hThread);

    // Wait for the child process to complete
    DWORD returnCode = WaitForSingleObject(pi.hProcess, INFINITE);
    LOGM_DEBUG(TAG, "JobEngine finished waiting for child process, returning %d", returnCode);

    // Get the exit code of the child process
    DWORD exitCode;
    if (!GetExitCodeProcess(pi.hProcess, &exitCode) || !stderrstream.toString().empty()) {
        LOGM_ERROR(TAG, "Could not get child process exit code: %d", GetLastError());
        exitCode = CMD_FAILURE;
    }

    // Clean up
    // Clean up
    CloseHandle(stdoutRead);
    CloseHandle(stderrRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return exitCode;
}
#endif

int JobEngine::exec_handlerScript(const std::string &command, PlainJobDocument::JobAction action)
{
    /**
     * \brief Create char array argv[] storing arguments to pass to execvp() function.
     * argv[0] executable path
     * argv[1] Linux user name
     * argv[2:] arguments required for executing the executable file..
     */
    int actionExecutionStatus;
    size_t argSize = 0;
    if (action.handlerInput->args.has_value())
    {
        argSize = action.handlerInput->args->size();
    }

    // cppcheck-suppress leakReturnValNotUsed
    std::unique_ptr<const char *[]> argv(new const char *[argSize + 3]);
    argv[0] = command.c_str();
    argv[1] = action.runAsUser->c_str();
    argv[argSize + 2] = nullptr;
    for (size_t i = 0; i < argSize; i++)
    {
        argv[i + 2] = action.handlerInput->args->at(i).c_str();
    }
    actionExecutionStatus = exec_cmd(argv);
    return actionExecutionStatus;
}

#ifndef _WIN32
bool JobEngine::verifySudoAndUser(PlainJobDocument::JobAction action)
{
    int execStatus1;
    // first to run command id $user and /bin/bash -c "command -v sudo" to verify user and sudo

    // cppcheck-suppress leakReturnValNotUsed
    std::unique_ptr<const char *[]> argv1(new const char *[3]);
    argv1[0] = "id";
    argv1[1] = action.runAsUser->c_str();
    argv1[2] = nullptr;

    execStatus1 = exec_process(argv1);

    if (execStatus1 == 0)
    {
        // cppcheck-suppress leakReturnValNotUsed
        std::unique_ptr<const char *[]> argv2(new const char *[4]);
        argv2[0] = "/bin/bash";
        argv2[1] = "-c";
        argv2[2] = "command -v sudo";
        argv2[3] = nullptr;

        int execStatus2 = exec_process(argv2);
        if (execStatus2 != 0)
        {
            return false;
        }
    }
    else
    {
        return false;
    }
    return true;
}
#else
// Windows implementation: verifies if specified user is an administrator or not
bool JobEngine::verifySudoAndUser(PlainJobDocument::JobAction action)
{
    if (action.runAsUser->empty())
        return false;
    else {
        HANDLE tokenHandle = NULL;
        BOOL result = false;

        // Assume the username is in the form "DOMAIN\Username"
        char domainName[256];
        char userName[256];
        DWORD domainNameSize = 256;
        DWORD userNameSize = (DWORD) action.runAsUser->length();

        // Split the domain and username
        if (!LookupAccountName(NULL, action.runAsUser->c_str(), NULL, &userNameSize, domainName, &domainNameSize, NULL)) {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                return false;
            }
            else {
                strcpy(domainName, ".");
            }
        }

        // Log on the user to get a token handle
        if (!LogonUser(userName, domainName, NULL, LOGON32_LOGON_BATCH, LOGON32_PROVIDER_DEFAULT, &tokenHandle)) {
            return false;
        }

        // Initialize the Administrators group SID
        PSID adminGroupSid = NULL;
        SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
        if (!AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroupSid)) {
            result = false;
        }

        // Check if the user token has the administrators group
        if (!CheckTokenMembership(tokenHandle, adminGroupSid, &result)) {
            result = false;
        }

        // Clean up
        if (adminGroupSid)
            FreeSid(adminGroupSid);
        CloseHandle(tokenHandle);
        return result;
    }
}
#endif

int JobEngine::exec_shellCommand(PlainJobDocument::JobAction action)
{
    int returnCode;
    bool verification;

    verification = verifySudoAndUser(action);

    if (!verification)
    {
        // if one of two verification fails, execute command without "sudo" and "$user"
        LOG_WARN(TAG, "username or sudo command not found");

        size_t argSize = action.commandInput->command.size();

        // cppcheck-suppress leakReturnValNotUsed
        std::unique_ptr<const char *[]> argv(new const char *[argSize + 1]);
        argv[argSize] = nullptr;
        for (size_t i = 0; i < argSize; i++)
        {
            argv[i] = action.commandInput->command.at(i).c_str();
        }
        // print out argv for debug
        for (size_t i = 0; i < argSize; ++i)
        {
            LOGM_DEBUG(TAG, "argv[%lu]: %s", i, (argv.get())[i]);
        }
        returnCode = exec_cmd(argv);
    }
    else
    {
        // if two verifications succeeds, build command using sudo -u $user -n $@ and execute
        size_t argSize = action.commandInput->command.size();

        // cppcheck-suppress leakReturnValNotUsed
        std::unique_ptr<const char *[]> argv(new const char *[argSize + 5]);
        argv[0] = "sudo";
        argv[1] = "-u";
        argv[2] = action.runAsUser->c_str();
        argv[3] = "-n";
        argv[argSize + 4] = nullptr;
        for (size_t i = 0; i < argSize; i++)
        {
            argv[i + 4] = action.commandInput->command.at(i).c_str();
        }
        // print out argv to debug
        for (size_t i = 0; i < argSize + 4; ++i)
        {
            LOGM_DEBUG(TAG, "argv[%lu]: %s", i, (argv.get())[i]);
        }

        returnCode = exec_cmd(argv);
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
