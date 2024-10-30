#ifndef AYR_FS_LINUX_LINUXLIB_HPP_
#define AYR_FS_LINUX_LINUXLIB_HPP_

#if defined(__linux__) || defined(__unix__)
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#endif