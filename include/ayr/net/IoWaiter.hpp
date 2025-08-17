#ifndef AYR_NET_SELECTOR_IOWAITER_HPP
#define AYR_NET_SELECTOR_IOWAITER_HPP

#include "../fs/oslib.h"

#if defined(AYR_WIN)
#include "Selector/Select.hpp"
namespace ayr
{
	using IoWaiter = Select;
}
#elif defined(AYR_LINUX)
#include "Selector/Epoll.hpp"
namespace ayr
{
	using IoWaiter = Epoll;
}
#else
#error "Unsupported platform"
#endif
#endif // AYR_NET_SELECTOR_IOWAITER_HPP