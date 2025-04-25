#ifndef AYR_FS_WIN_WINLIB_HPP
#define AYR_FS_WIN_WINLIB_HPP

#include "../../base/Object.hpp"

#include <WinSock2.h>
#include <Windows.h>
#include <fileapi.h>
#include <WS2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#undef max
#undef min

namespace ayr
{
	// 错误码转化为字符串
	CString errorno2str(int errorno)
	{
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
	}

	CString get_error_msg() { return errorno2str(GetLastError()); }
}
#endif // AYR_FS_WIN_WINLIB_HPP