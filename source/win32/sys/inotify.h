// sys/inotify.h
// stab include to reduce number of #ifdef around #include directives under Windows

#ifndef _WIN32

#pragma message("this inotify.h implementation is for Windows only!")

#else

#ifndef __INOTIFY_H__
#define __INOTIFY_H__
#endif /*__INOTIFY_H__*/

#endif
