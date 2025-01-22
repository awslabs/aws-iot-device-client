#include "wait.h"
#include "unistd.h"

pid_t waitpid(pid_t pid, int *status, int options)
{
    HANDLE hProcess;
    DWORD dwExitCode;

    // Open the process
    hProcess = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION, FALSE, (DWORD)pid);
    if (hProcess == NULL) {
        printf("OpenProcess failed (%d).\n", GetLastError());
        return -1;
    }

    // Wait for the process to terminate
    DWORD dwReturnValue;
    if (options & WNOHANG) {
        // If WNOHANG is set, return immediately if the process is still running
        dwReturnValue = WaitForSingleObject(hProcess, 0);
    } else {
        // Otherwise, wait indefinitely for the process to terminate
        dwReturnValue = WaitForSingleObject(hProcess, INFINITE);
    }

    if (dwReturnValue != WAIT_OBJECT_0 && !(options & WNOHANG)) {
        // If WNOHANG is set, return 0 if the process is still running
        printf("WaitForSingleObject failed (%d).\n", GetLastError());
        return -1;
    } else if (dwReturnValue == WAIT_TIMEOUT && (options & WNOHANG)) {
        // If WNOHANG is set and the process is still running, return 0
        return 0;
    }

    // Get the exit code of the process
    if (!GetExitCodeProcess(hProcess, &dwExitCode)) {
        printf("GetExitCodeProcess failed (%d).\n", GetLastError());
        return -1;
    }

    // Close process handle
    CloseHandle(hProcess);

    // Set status if required
    if (status != NULL) {
        *status = (int)dwExitCode;
    }

    return pid;
}

int waitid(idtype_t idtype, int id, siginfo_t *infop, int options) 
{
    DWORD dwId;
    HANDLE hProcess;
    DWORD dwExitCode;

    switch (idtype) {
        case P_PID:
            dwId = id;
            break;
        case P_PGID:
            // Not supported in Windows
            return -1;
        case P_ALL:
            // Not supported in Windows
            return -1;
        default:
            return -1;
    }

    hProcess = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION, FALSE, dwId);
    if (hProcess == NULL) {
        return -1;
    }

    if (options & WNOHANG) {
        // Check if the process is still running
        DWORD waitResult = WaitForSingleObject(hProcess, 0);
        if (waitResult == WAIT_TIMEOUT) {
            CloseHandle(hProcess);
            return 0; // Process is still running
        }
    } else {
        // Wait for the process to exit
        WaitForSingleObject(hProcess, INFINITE);
    }

    GetExitCodeProcess(hProcess, &dwExitCode);

    infop->si_pid = dwId;
    infop->si_status = dwExitCode;

    CloseHandle(hProcess);

    return 0;
}


