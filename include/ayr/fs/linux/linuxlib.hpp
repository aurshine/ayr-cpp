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
		CString res(256);
		std::snprintf(res.data(), 256, "errno %d: %s\n", errorno, strerror(errorno));
		return res;
	}

	CString get_error_msg() { return errorno2str(errno); }
}
#endif