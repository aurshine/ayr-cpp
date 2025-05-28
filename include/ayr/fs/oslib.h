#ifndef AYR_FS_OSLIB_H
#define AYR_FS_OSLIB_H

#include "../base/Object.hpp"
#include "../base/raise_error.hpp"

#if defined(_WIN32) || defined(_WIN64)
#define AYR_WIN

#include <WinSock2.h>
#include <Windows.h>
#include <fileapi.h>
#include <WS2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#undef max
#undef min

#elif defined(__linux__) || defined(__unix__)
#define AYR_LINUX

#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <unistd.h>

#elif defined(__APPLE__)
#define AYR_MAC

#endif // 平台判断

namespace ayr
{
	// 错误码转化为字符串
	CString errorno2str(int errorno)
	{
#if defined(AYR_WIN)
		CString error_msg{ 216 }, res{ 256 };
		FormatMessageA(
			FORMAT_MESSAGE_FROM_SYSTEM,
			nullptr,
			errorno,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			error_msg.data(),
			216,
			nullptr
		);

		std::snprintf(res.data(), 256, "errno %d: %s\n", errorno, error_msg.data());
		return res;
#elif defined(AYR_LINUX) || defined(AYR_MAC)
		CString res(256);
		std::snprintf(res.data(), 256, "errno %d: %s\n", errorno, strerror(errorno));
		return res;
#endif // 平台判断
	}

	CString get_error_msg()
	{
#if defined(AYR_WIN)
		return errorno2str(GetLastError());
#elif defined(AYR_LINUX) || defined(AYR_MAC)
		return errorno2str(errno);
#endif // 平台判断
	}
}
#endif  // AYR_FS_OSLIB_H