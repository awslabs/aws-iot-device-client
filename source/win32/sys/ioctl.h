// sys/ioctl.h
// stab include to reduce number of #ifdef around #include directives under Windows

#ifndef _WIN32

#pragma message("this ioctl.h implementation is for Windows only!")

#else

#ifndef __IOCTL_H__
#define __IOCTL_H__
#endif /*__IOCTL_H__*/

#endif
