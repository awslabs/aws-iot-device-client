// arpa/inet.h
// stab include file to reduce number of #ifdef around #include directives under Windows
#ifndef _WIN32

#pragma message("this inet.h implementation is for Windows only!")

#else

#ifndef __INET_H__
#define __INET_H__

#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#endif /*__INET_H__*/

#endif
