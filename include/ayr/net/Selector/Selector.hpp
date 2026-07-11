#ifndef AYR_NET_SELECTOR_HPP
#define AYR_NET_SELECTOR_HPP

#include "../../fs/oslib.h"

#if defined(AYR_WIN)
#include "IOCP/IOCP.hpp"

namespace ayr
{
	namespace net
	{
		using Selector = IOCP;
	}
}
#elif defined(AYR_LINUX)
#include "EPOLL/Epoll.hpp"
namespace ayr
{
	namespace net
	{
		using Selector = Epoll;
	}
}
#else
#error "Unsupported platform"
#endif
#endif // AYR_NET_SELECTOR_HPP