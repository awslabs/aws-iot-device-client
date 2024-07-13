// sys/sys_types.h
// Defining Linux-style permissions or mapping them to Windows permissions

#ifndef _WIN32

#pragma message("this sys_types.h implementation is for Windows only!")

#else

#ifndef __SYS_TYPES_H__
#define __SYS_TYPES_H__

#include <stdint.h>
#include "../linux/limits.h"

typedef int pid_t;
typedef int gid_t;
typedef int uid_t;
typedef int sigval_t;
typedef int sigset_t;
typedef unsigned short ushort;
typedef	int key_t;
typedef intptr_t ssize_t;
typedef unsigned short mode_t;
typedef int gid_t;
typedef int Atom;

enum
{	F_DUPFD, 
	F_GETFD,
	F_SETFD, 
	F_GETFL, 
	F_SETFL, 
	F_GETLK, 
	F_SETLK,
	F_SETLKW,
	FD_CLOEXEC
};

/* permission bits below must be defined in sys/stat.h, but MSVC lacks them */
#ifndef S_IRWXU
#define S_IRWXU 0700
#endif /* S_IRWXU */

#ifndef S_IRUSR
#define S_IRUSR 0400
#endif /* S_IRUSR */

#ifndef S_IWUSR
#define S_IWUSR 0200
#endif /* S_IWUSR */

#ifndef S_IXUSR
#define S_IXUSR 0100
#endif /* S_IXUSR */

#ifndef S_IRWXG
#define S_IRWXG 070
#endif /* S_IRWXG */

#ifndef S_IRGRP
#define S_IRGRP 040
#endif /* S_IRGRP */

#ifndef S_IWGRP
#define S_IWGRP 020
#endif /* S_IWGRP */

#ifndef S_IXGRP
#define S_IXGRP 010
#endif /* S_IXGRP */

#ifndef S_IRWXO
#define S_IRWXO 07
#endif /* S_IRWXO */

#ifndef S_IROTH
#define S_IROTH 04
#endif /* S_IROTH */

#ifndef S_IWOTH
#define S_IWOTH 02
#endif /* S_IWOTH */

#ifndef S_IXOTH
#define S_IXOTH 01
#endif /* S_IXOTH */

#ifndef S_ISUID
#define S_ISUID 04000
#endif /* S_ISUID */

#ifndef S_ISGID
#define S_ISGID 02000
#endif /* S_ISGID */

#ifndef S_ISVTX
#define S_ISVTX 01000
#endif /* S_ISVTX */

#ifndef S_IRWXUGO
#define S_IRWXUGO 0777
#endif /* S_IRWXUGO */

#ifndef S_IALLUGO
#define S_IALLUGO 0777
#endif /* S_IALLUGO */	

#ifndef S_IRUGO
#define S_IRUGO 0444
#endif /* S_IRUGO */

#ifndef S_IWUGO
#define S_IWUGO 0222
#endif /* S_IWUGO */

#ifndef S_IXUGO
#define S_IXUGO 0111
#endif /* S_IXUGO */

#ifndef _S_IFMT
#define _S_IFMT 0xF000
#endif /* _S_IFMT */

#ifndef _S_IFIFO
#define _S_IFIFO 0x1000
#endif /* _S_IFIFO */

#ifndef _S_IFCHR
#define _S_IFCHR 0x2000
#endif /* _S_IFCHR */

#ifndef _S_IFDIR
#define _S_IFDIR 0x4000
#endif /* _S_IFDIR */

#ifndef _S_IFBLK
#define _S_IFBLK 0x6000
#endif /* _S_IFBLK */

#ifndef _S_IFREG
#define _S_IFREG 0x8000
#endif /* _S_IFREG */

#ifndef _S_IFLNK
#define _S_IFLNK 0xA000
#endif /* _S_IFLNK */

#ifndef _S_IFSOCK
#define _S_IFSOCK 0xC000
#endif /* _S_IFSOCK */

#ifndef S_IFMT
#define S_IFMT _S_IFMT
#endif /* S_IFMT */

#ifndef S_IFIFO
#define S_IFIFO _S_IFIFO
#endif /* S_IFIFO */

#ifndef S_IFCHR
#define S_IFCHR _S_IFCHR
#endif /* S_IFCHR */

#ifndef S_IFDIR
#define S_IFDIR _S_IFDIR
#endif /* S_IFDIR */

#ifndef S_IFBLK
#define S_IFBLK _S_IFBLK
#endif /* S_IFBLK */

#ifndef S_IFREG
#define S_IFREG _S_IFREG
#endif /* S_IFREG */

#ifndef S_IFLNK
#define S_IFLNK _S_IFLNK
#endif /* S_IFLNK */

#ifndef S_IFSOCK
#define S_IFSOCK _S_IFSOCK
#endif /* S_IFSOCK */

#ifndef S_ISTYPE
#define S_ISTYPE(mode, mask) (((mode) & S_IFMT) == (mask))
#endif /* S_ISTYPE */

#ifndef S_ISFIFO
#define S_ISFIFO(mode) S_ISTYPE(mode, S_IFIFO)
#endif /* S_ISFIFO */

#ifndef S_ISCHR
#define S_ISCHR(mode) S_ISTYPE(mode, S_IFCHR)
#endif /* S_ISCHR */

#ifndef S_ISDIR
#define S_ISDIR(mode) S_ISTYPE(mode, S_IFDIR)
#endif /* S_ISDIR */

#ifndef S_ISBLK
#define S_ISBLK(mode) S_ISTYPE(mode, S_IFBLK)
#endif /* S_ISBLK */

#ifndef S_ISREG
#define S_ISREG(mode) S_ISTYPE(mode, S_IFREG)
#endif /* S_ISREG */

#ifndef S_ISLNK
#define S_ISLNK(mode) S_ISTYPE(mode, S_IFLNK)
#endif /* S_ISLNK */

#ifndef S_ISSOCK
#define S_ISSOCK(mode) S_ISTYPE(mode, S_IFSOCK)
#endif /* S_ISSOCK */

// #define PATH_MAX 255

#define EBADFD 200
#define ESHUTDOWN 201
#define SHUT_RD SD_RECEIVE
#define SHUT_WR SD_SEND
#define SHUT_RDWR SD_BOTH

#define MSG_NOSIGNAL 0
#if 0
#define TCP_KEEPCNT 0
#endif

#define access _access

#define F_GETFL 0
#define F_SETFL 0
#define O_NONBLOCK 0
#define O_SYNC 0
#define O_NOCTTY 0

#endif /*__SYS_TYPES_H__*/
#endif