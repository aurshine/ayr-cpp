#ifndef AYR_NET_SELECTOR_IOWAITER_HPP
#define AYR_NET_SELECTOR_IOWAITER_HPP

#include "../../fs/oslib.h"

#if defined(AYR_WIN)
#include "Select.hpp"
namespace ayr
{
	namespace net
	{
		using IoWaiter = Select;
	}
}
#elif defined(AYR_LINUX)
#include "Epoll.hpp"
namespace ayr
{
	namespace net
	{
		using IoWaiter = Epoll;
	}
}
#else
#error "Unsupported platform"
#endif
#endif // AYR_NET_SELECTOR_IOWAITER_HPP