/*
MIT License
Copyright (c) 2019 win32ports
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

//#pragma once

#ifndef __UNISTD_H__
#define __UNISTD_H__

#ifndef _WIN32

#pragma message("this unistd.h implementation is for Windows only!")

#else /* _WIN32 */

#include <direct.h> // For _mkdir
#include <io.h>     // For _chmod
#include "sys/sys_types.h"
#include <windows.h>
#include <sddl.h>
#include <fcntl.h>    /* _O_BINARY */

#define _PC_PATH_MAX  32767  // Maximum path length for Windows UNC paths


#ifndef _INC_IO
#include <io.h> /* _access() */
#endif /* _INC_IO */

#ifndef _INC_DIRECT
#include <direct.h> /* _chdir() */
#endif /* _INC_DIRECT */

#ifndef _INC_PROCESS
#include <process.h> /* _execl() */
#endif /* _INC_PROCESS */

#include <sys/stat.h> /* */

#ifndef access
#define access _access
#endif /* access */

#ifndef R_OK
#define R_OK 04
#endif /* R_OK */

#ifndef W_OK
#define W_OK 02
#endif /* W_OK */

#ifndef X_OK
#define X_OK R_OK
#endif /* X_OK */

#ifndef F_OK
#define F_OK 00
#endif /* F_OK */

#ifndef chdir
#define chdir _chdir
#endif /* chdir */

#ifndef close
#define close _close
#endif /* close */

#ifndef read
#define read _read
#endif /* read */

int win_chmod(const char *filename, mode_t mode);
#ifndef chmod
#define chmod win_chmod
#endif /* chmod */

int win_stat(const char *filename, struct stat *buffer);
#define stat(filename, buffer) win_stat(filename, buffer)

#ifndef open
#define open _open
#endif /* open */

#define fdopen _fdopen
#define pipe(pipes) _pipe((pipes),8*1024,_O_BINARY)
#define execvp _execvp

#define strdup _strdup

#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif /* STDIN_FILENO */

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif /* STDOUT_FILENO */

#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif /* STDERR_FILENO */

#ifndef dup
#define dup _dup
#endif /* dup */

#ifndef dup2
#define dup2 _dup2
#endif /* dup2 */

#ifndef execl
#define execl _execl
#endif /* execl */

#ifndef execle
#define execle _execle
#endif /* execle */

#ifndef execlp
#define execlp _execlp
#endif /* execlp */

#ifndef execp
#define execp _execp
#endif /* execp */

#ifndef execpe
#define execpe _execpe
#endif /* execpe */

#ifndef execpp
#define execpp _execpp
#endif /* execpp */

#ifndef rmdir
#define rmdir _rmdir
#endif /* rmdir */

#ifndef unlink
#define unlink _unlink
#endif /* unlink */

inline
int mkdir2(const char *pathname, mode_t mode) {
    if (_mkdir(pathname) == 0) {
//        _chmod(pathname, mode); // Set permissions separately
        win_chmod(pathname, mode);
        return 0; // Success
    } else {
        return -1; // Error
    }
}

#ifndef mkdir
#define mkdir mkdir2
#endif /* mkdir*/

#ifndef getpid
#define getpid _getpid
#endif /* getpid */
inline
uid_t getuid() {
    HANDLE hToken;
    DWORD dwLengthNeeded;
    PTOKEN_USER pTokenUser;
    TCHAR lpUserName[256];
    DWORD dwUserNameSize = sizeof(lpUserName);
    char domain[256];
    DWORD domainSize = sizeof(domain);
    SID_NAME_USE use;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        return -1;
    }

    if (!GetTokenInformation(hToken, TokenUser, NULL, 0, &dwLengthNeeded) &&
        GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        CloseHandle(hToken);
        return -1;
    }

    pTokenUser = (PTOKEN_USER)GlobalAlloc(GPTR, dwLengthNeeded);
    if (!pTokenUser) {
        CloseHandle(hToken);
        return -1;
    }

    if (!GetTokenInformation(hToken, TokenUser, pTokenUser, dwLengthNeeded, &dwLengthNeeded)) {
        GlobalFree(pTokenUser);
        CloseHandle(hToken);
        return -1;
    }

    if (!LookupAccountSid(NULL, pTokenUser->User.Sid, lpUserName, &dwUserNameSize, domain, &domainSize, &use)) {
        GlobalFree(pTokenUser);
        CloseHandle(hToken);
        return -1;
    }

    // Convert SID to string representation
    LPTSTR stringSid;
    if (!ConvertSidToStringSid(pTokenUser->User.Sid, &stringSid)) {
        GlobalFree(pTokenUser);
        CloseHandle(hToken);
        return -1;
    }

    // Parse the string SID to extract the user ID
    uid_t uid = atoi(stringSid);
    
    // Free resources
    GlobalFree(pTokenUser);
    LocalFree(stringSid);
    CloseHandle(hToken);

    return uid;
}

#ifndef SIGINT
#define SIGINT 2
#endif /* SIGINT */
#ifndef SIGTERM
#define SIGTERM 15
#endif /* SIGTERM */
inline
int kill(pid_t pid, int sig) {
    DWORD dwCtrlEvent;

    switch (sig) {
        case SIGINT:
            dwCtrlEvent = CTRL_C_EVENT;
            break;
        case SIGTERM:
            dwCtrlEvent = CTRL_BREAK_EVENT;
            break;
        default:
            // Unsupported signal
            return -1;
    }

    if (pid == 0) {
        // Send the signal to all processes in the current group
        if (!GenerateConsoleCtrlEvent(dwCtrlEvent, 0)) {
            return -1;
        }
    } else {
        // Send the signal to the specified process
        if (!GenerateConsoleCtrlEvent(dwCtrlEvent, pid)) {
            return -1;
        }
    }

    return 0;
}

inline
long pathconf(const char *path, int name) {
    if (name != _PC_PATH_MAX) {
        return -1; // Unsupported parameter
    }

    char volume[MAX_PATH];
    DWORD max_path_length;
    
    if (!GetVolumeInformation(path, NULL, 0, NULL, &max_path_length, NULL, volume, MAX_PATH)) {
        return -1;
    }
    
    return max_path_length;
}

inline
int setenv(const char *name, const char *value, int overwrite) {
    // If overwrite is 0 and the environment variable already exists, do nothing
    if (!overwrite && getenv(name) != NULL) {
        return 0;
    }
    
    // Construct the environment variable string
    size_t len = strlen(name) + strlen(value) + 2;
    char *env_var = (char*)malloc(len);
    if (env_var == NULL) {
        return -1; // Memory allocation failed
    }
    snprintf(env_var, len, "%s=%s", name, value);
    
    // Set the environment variable using _putenv_s
    int result = _putenv_s(name, env_var);
    
    free(env_var);
    
    return result == 0 ? 0 : -1;
}

inline
pid_t vfork()
{
    HANDLE hProcess;
    HANDLE hThread;
    DWORD dwThreadId;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    // Initialize the STARTUPINFO structure.
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

    // Create the child process.
    if (!CreateProcess(NULL,   // No module name (use command line)
        (LPSTR)"child.exe",           // Command line
        NULL,                   // Process handle not inheritable
        NULL,                   // Thread handle not inheritable
        FALSE,                  // Set handle inheritance to FALSE
        CREATE_SUSPENDED |      // Create the process in a suspended state
        CREATE_NEW_CONSOLE,     // Create a new console window for the new process
        NULL,                   // Use parent's environment block
        NULL,                   // Use parent's starting directory
        &si,                    // Pointer to STARTUPINFO structure
        &pi))                   // Pointer to PROCESS_INFORMATION structure
    {
        printf("CreateProcess failed (%d).\n", GetLastError());
        return -1;
    }

    // Now child process is suspended, we can attach to it.
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pi.dwProcessId);
    if (hProcess == NULL) {
        printf("OpenProcess failed (%d).\n", GetLastError());
        return -1;
    }

    // Resume child process
    hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)ResumeThread, NULL, 0, &dwThreadId);
    if (hThread == NULL) {
        printf("CreateRemoteThread failed (%d).\n", GetLastError());
        return -1;
    }

    // Wait for child process to finish
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Close process and thread handles.
    CloseHandle(hThread);
    CloseHandle(hProcess);

    return pi.dwProcessId;	
}

#endif /* _WIN32 */

#endif /* __UNISTD_H__*/