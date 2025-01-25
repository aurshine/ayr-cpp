#ifndef AYR_FS_LINUX_LINUXLIB_HPP_
#define AYR_FS_LINUX_LINUXLIB_HPP_

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../../base/CString.hpp"
#include "../../base/raise_error.hpp"

namespace ayr
{
	CString errorno2str(int errorno)
	{
		CString error_msg{ 256 };
		strerror_r(errorno, error_msg.data(), 256);
		return error_msg;
	}

	CString get_error_msg() { return errorno2str(errno); }
}
#endif