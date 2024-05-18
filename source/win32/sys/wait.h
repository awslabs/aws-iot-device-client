// sys/wait.h
// Copyright 2016 Robin.Rowe@CinePaint.org
// License open source MIT

#ifndef sys_wait_h
#define sys_wait_h

#include <Windows.h>
#include "sys_types.h"
//#include "../portable/stub.h"
//#include "sigaction.h"
#include "../unistd.h"

#define WIFEXITED(wstatus) 1
#define WEXITSTATUS(wstatus) 0
#define WIFSIGNALED(status)    (((status) & 0xFF) != 0 && ((status) & 0x7F) != 0)
#define WTERMSIG(status)       (((status) & 0xFF))
#define WIFSTOPPED(status)     (((status) & 0xFF) == 0x7F)
#define WSTOPSIG(status)       (((status) >> 8) & 0xFF)

#define WNOHANG 0

inline
pid_t wait(int *status)
{
    DWORD dwExitCode;

    // Wait for any child process to terminate
    DWORD dwReturnValue = WaitForSingleObject(GetCurrentProcess(), INFINITE);
    if (dwReturnValue != WAIT_OBJECT_0) {
        printf("WaitForSingleObject failed (%d).\n", GetLastError());
        return -1;
    }

    // Get the exit code of the process
    if (!GetExitCodeProcess(GetCurrentProcess(), &dwExitCode)) {
        printf("GetExitCodeProcess failed (%d).\n", GetLastError());
        return -1;
    }

    // Set status if required
    if (status != NULL) {
        *status = (int)dwExitCode;
    }

    return getpid();
}

inline
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

typedef int idtype_t;
typedef int id_t;

typedef struct {
    int si_signo;       // Signal number
    int si_errno;       // An errno value
    int si_code;        // Signal code
    pid_t si_pid;       // PID of sender
    uid_t si_uid;       // Real UID of sender
    int si_status;      // Exit value or signal
    clock_t si_utime;   // User CPU time consumed
    clock_t si_stime;   // System CPU time consumed
    // Additional fields may be present depending on the signal and the context in which it was delivered
} siginfo_t;

#define P_PID 1
#define P_PGID 2
#define P_ALL 3

inline
int waitid(idtype_t idtype, int id, siginfo_t *infop, int options) {
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


#endif
