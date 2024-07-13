// sys/socket.h
// stab include to reduce number of #ifdef around #include directives under Windows

#ifndef _WIN32

#pragma message("this socket.h implementation is for Windows only!")

#else

#ifndef __SOCKET_H__
#define __SOCKET_H__
#endif /*__SOCKET_H__*/

#endif
