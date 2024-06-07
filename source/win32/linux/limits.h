// linux/limits.h
// stab include file to reduce number of #ifdef around #include directives under Windows
// and to declare missing definitions from Linux's version of limits.h
#ifndef _WIN32

#pragma message("this limits.h implementation is for Windows only!")

#else

#ifndef __LIMITS_H__
#define __LIMITS_H__

#define PATH_MAX   _MAX_PATH

#endif /*__LIMITS_H__*/

#endif