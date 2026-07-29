#ifndef AYR_FS_OSLIB_H
#define AYR_FS_OSLIB_H

#include <limits.h>

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

#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#if defined(__APPLE__)
#define AYR_MAC
#else
#define AYR_LINUX
#endif

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

	/**
	 * @brief 将 C 运行库或 POSIX errno 错误码转换为可读字符串。
	 * 
	 * @param error errno 错误码。
	 * 
	 * @return 包含数值错误码和 strerror 说明的 CString。
	 * 
	 * @note Windows 的 errno 与 GetLastError、WSAGetLastError 属于不同错误域，不能混用。
	 */
	def c_error2str(int error) -> CString
	{
		return _os_error::message(static_cast<unsigned long>(error), std::strerror(error));
	}

#if defined(AYR_WIN)
	/**
	 * @brief 将 Win32 API 错误码转换为可读字符串。
	 * 
	 * @param error GetLastError() 或 Win32 API 返回的错误码。
	 * 
	 * @return 包含数值错误码和系统说明的 CString。
	 */
	def win_error2str(DWORD error) -> CString
	{
		return _os_error::windows_message(error);
	}
#endif

	/**
	 * @brief 获取当前平台普通系统 API 的最近一次错误消息。
	 * 
	 * @return Windows 下格式化 GetLastError()，POSIX 下格式化 errno。
	 * 
	 * @note 应在失败的系统调用之后立即调用，避免错误状态被其他 API 覆盖。
	 */
	def get_system_error_msg() -> CString
	{
#if defined(AYR_WIN)
		return win_error2str(GetLastError());
#elif defined(AYR_LINUX) || defined(AYR_MAC)
		return c_error2str(errno);
#endif // 平台判断
	}

	/**
	 * @brief 获取当前平台 socket API 的最近一次错误消息。
	 * 
	 * @return Windows 下格式化 WSAGetLastError()，POSIX 下格式化 errno。
	 */
	def get_socket_error_msg() -> CString
	{
#if defined(AYR_WIN)
		return win_error2str(WSAGetLastError());
#elif defined(AYR_LINUX) || defined(AYR_MAC)
		return c_error2str(errno);
#endif
	}

	/**
	 * @brief 将 getaddrinfo 返回的 EAI_* 错误码转换为可读字符串。
	 * 
	 * @param error getaddrinfo 直接返回的错误码。
	 * 
	 * @return 包含 EAI 错误码和 gai_strerror 说明的 CString。
	 * 
	 * @note getaddrinfo 使用独立错误域，不读取 errno 或 WSAGetLastError。
	 */
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
