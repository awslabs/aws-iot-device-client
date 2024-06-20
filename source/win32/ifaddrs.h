/**
 * Windows implemenatation of ifaddrs.h (limited functioanlity)
*/
#ifndef _WIN32

#pragma message("this ifaddrs.h implementation is for Windows only!")

#else
#ifndef __IFADDRS_H__
#define __IFADDRS_H__

#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>

#pragma comment(lib, "iphlpapi.lib")

#endif /* __IFADDRS_H__  */
#endif