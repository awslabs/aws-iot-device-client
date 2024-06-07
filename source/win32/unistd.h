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

/**
 * @brief Changes file permissions
 * 
 * @param filename - file location
 * @param mode - linux style permissions
 * @return int - 0 - success. -1 - for error.
 */
int win_chmod(const char *filename, mode_t mode);
#ifndef chmod
#define chmod win_chmod
#endif /* chmod */

/**
 * @brief Returns file status
 * 
 * @param filename - file location
 * @param buffer - pointer to a resulting file state structure
 * @return int - 0 - success. -1 - for error.
 */
int win_stat(const char *filename, struct stat *buffer);
#define stat(filename, buffer) win_stat(filename, buffer)

/**
 * @brief Opens and possible creates a dile
 * 
 * @param _FileName - path to the file to be opened
 * @param _OFlag - operation flags: O_RDONLY, O_WRONLY, O_RDWR
 * @param _PMode - file mode
 * @return int - file descriptor
 */
int win_open(
    _In_z_ char const* const _FileName,
    _In_   int         const _OFlag,
    _In_   int         const _PMode = 0
);
#ifndef open
#define open win_open
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

/**
 * @brief Create a directory at the specified path
 * 
 * @param pathname - directory full path
 * @param mode - Linux style permissions for the newly created file item
 * @return int - result of the operation
 */
int win_mkdir(const char *pathname, mode_t mode);
#ifndef mkdir
#define mkdir win_mkdir
#endif /* mkdir*/

#ifndef getpid
#define getpid _getpid
#endif /* getpid */

/**
 * @brief Get user ID for the current process
 * 
 * @return uid_t - user ID
 */
uid_t getuid();

int kill(pid_t pid, int sig);
/**
 * @brief gets a value for configuration option name for the filename path.
 * 
 * @param path - filename to use to obtain configuration option
 * @param name - name of the configuration option. _PC_PATH_MAX is the only one supported in the current implementation.
 * @return long - maximum for the specified option
 */
long pathconf(const char *path, int name);

/**
 * @brief Set the value of environment variable
 * 
 * @param name - variable name
 * @param value = the value
 * @param overwrite = pass true to overwrite existing variable value or false - to keep existing value
 * @return int - operation result
 */
int setenv(const char *name, const char *value, int overwrite);

#endif /* _WIN32 */

#endif /* __UNISTD_H__*/