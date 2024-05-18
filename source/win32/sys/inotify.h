// sys/inotify.h
// Copyright 2016 Robin.Rowe@CinePaint.org
// License open source MIT

#ifndef sys_inotify_h
#define sys_inotify_h

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#else
#define inline __inline
#endif

#define IN_ISDIR        0x40000000  /* Event occurred against directory */

enum
{	IN_NONBLOCK,
	IN_CLOEXEC,
	IN_ACCESS,
	IN_ATTRIB,
	IN_CLOSE_WRITE,
	IN_CLOSE_NOWRITE,
	IN_CREATE,
	IN_DELETE,
	IN_DELETE_SELF,
	IN_MODIFY,
	IN_MOVE_SELF,
	IN_MOVED_FROM,
	IN_MOVED_TO,
        IN_OPEN,
        IN_MOVE
};

struct inotify_event 
{	int  wd;       /* Watch descriptor */
	uint32_t mask;     /* Mask describing event */
	uint32_t cookie;   /* Unique cookie associating related events (for rename(2)) */
	uint32_t len;      /* Size of name field */
	char  name[1];   /* Optional null-terminated name */
};

inline
int inotify_init(void)
{
	return -1;
}

inline
int inotify_init1(int /*flags*/)
{
	return -1;
}

inline
int inotify_add_watch(int /*fd*/, const char */*pathname*/, uint32_t /*mask*/)
{
	return -1;
}

inline
int inotify_rm_watch(int /*fd*/, int /*wd*/)
{
	return -1;
}

#ifdef __cplusplus
}
#endif

#endif
