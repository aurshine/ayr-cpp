#ifndef AYR_FS_OSLIB_H
#define AYR_FS_OSLIB_H

#if defined(_WIN32) || defined(_WIN64)
#define AYR_WIN

#include <io.h>
#include <winsock2.h>
#include <mswsock.h>
#include <windows.h>
#include <fileapi.h>
#include <WS2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#undef max
#undef min

#elif defined(__linux__) || defined(__unix__)
#define AYR_LINUX

#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>

#elif defined(__APPLE__)
#define AYR_MAC

#endif // 平台判断

#include "../base/raise_error.hpp"

namespace ayr
{
	namespace _os_error
	{
		def message(unsigned long error, const char* description, c_size description_size = -1) -> CString
		{
			Buffer buf;
			buf  << error;
			if (description != nullptr && description_size != 0)
			{
				if (description_size < 0)
					description_size = std::strlen(description);

				// FormatMessage生成的文本通常以CR/LF结尾，不把它们带入错误信息。
				while (description_size > 0 &&
					(description[description_size - 1] == '\r' ||
					 description[description_size - 1] == '\n' ||
					 description[description_size - 1] == ' '))
					--description_size;

				if (description_size > 0)
				{
					buf << ": ";
					buf.append_bytes(description, description_size);
				}
			}
			return from_buffer(std::move(buf));
		}

#if defined(AYR_WIN)
		def windows_message(DWORD error) -> CString
		{
			char* description = nullptr;
			DWORD description_size = FormatMessageA(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr,
				error,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				reinterpret_cast<char*>(&description),
				0,
				nullptr
			);

			CString result = message(error, description, description_size);
			if (description != nullptr)
				LocalFree(description);
			return result;
		}
#endif
	}

	/*
	* C运行库/Linux errno错误。
	* Windows的errno与GetLastError、WSAGetLastError属于不同的错误域，不能混用。
	*/
	def c_error2str(int error) -> CString
	{
		return _os_error::message(static_cast<unsigned long>(error), std::strerror(error));
	}

#if defined(AYR_WIN)
	// 普通Win32 API返回或GetLastError取得的错误。
	def win_error2str(DWORD error) -> CString
	{
		return _os_error::windows_message(error);
	}
#endif

	// 获取当前平台普通系统API的最后错误。
	def get_system_error_msg() -> CString
	{
#if defined(AYR_WIN)
		return win_error2str(GetLastError());
#elif defined(AYR_LINUX) || defined(AYR_MAC)
		return c_error2str(errno);
#endif // 平台判断
	}

	// 获取当前平台socket API的最后错误。
	def get_socket_error_msg() -> CString
	{
#if defined(AYR_WIN)
		return win_error2str(WSAGetLastError());
#elif defined(AYR_LINUX) || defined(AYR_MAC)
		return c_error2str(errno);
#endif
	}

	// getaddrinfo返回独立的EAI_*错误码，不应读取errno或WSAGetLastError。
	def gai_error2str(int error) -> CString
	{
#if defined(AYR_WIN)
		const char* description = gai_strerrorA(error);
#else
		const char* description = gai_strerror(error);
#endif
		Buffer buf;
		buf << "getaddrinfo error " << error;
		if (description != nullptr)
			buf << ": " << description;
		return from_buffer(std::move(buf));
	}

#if defined(AYR_WIN)
	using BaseSocket = SOCKET;
#elif defined(AYR_LINUX) || defined(AYR_MAC)
	using BaseSocket = int;
#endif // 平台判断

}
#endif  // AYR_FS_OSLIB_H
