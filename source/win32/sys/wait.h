// sys/wait.h
// Copyright 2016 Robin.Rowe@CinePaint.org
// License open source MIT

#ifndef sys_wait_h
#define sys_wait_h

#define WIFEXITED(wstatus) 1
#define WEXITSTATUS(wstatus) 0
#define WIFSIGNALED(status)    (((status) & 0xFF) != 0 && ((status) & 0x7F) != 0)
#define WTERMSIG(status)       (((status) & 0xFF))
#define WIFSTOPPED(status)     (((status) & 0xFF) == 0x7F)
#define WSTOPSIG(status)       (((status) >> 8) & 0xFF)

#define WNOHANG 0

/**
 * @brief Wait for the specified process to stop or terminate. Simplified implementation for Windows.
 * 
 * @param pid - process ID
 * @param status - process status. No status is returned if NULL is provided for the status
 * @param options - wait options. Only WNOHANG is supported. Otherwise it waits indefinitely 
 * @return pid_t - -1 is returned in case of errors, 0 if WNOHANG is passed and process is finished. pid is returned otherwise.
 */
pid_t waitpid(pid_t pid, int *status, int options);

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

int waitid(idtype_t idtype, int id, siginfo_t *infop, int options);

#endif
