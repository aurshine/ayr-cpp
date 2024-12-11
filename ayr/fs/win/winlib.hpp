#ifndef AYR_FS_WIN_WINLIB_HPP
#define AYR_FS_WIN_WINLIB_HPP

#include <WinSock2.h>
#include <Windows.h>
#include <fileapi.h>
#include <WS2tcpip.h>

#include "../../base/CString.hpp"

#pragma comment(lib, "Ws2_32.lib")

#undef max
#undef min

namespace ayr
{
	// 错误码转化为字符串
	CString errorno2str(int errorno)
	{
		CString error_msg{ 256 };
		FormatMessageA(
			FORMAT_MESSAGE_FROM_SYSTEM,
			nullptr,
			errorno,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			error_msg.data(),
			256,
			nullptr
		);
		return error_msg;
	}

	CString get_error_msg() { return errorno2str(GetLastError()); }
}
#endif // AYR_FS_WIN_WINLIB_HPP