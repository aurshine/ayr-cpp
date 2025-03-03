#include "ayr/coro.hpp"
#include "ayr/base/CString.hpp"

using namespace ayr;
using namespace coro;

Task<void> hello()
{
	print("into hello, wait for 2s");
	co_await coro::Sleep(2s);
	print("hello");
}

Task<void> world()
{
	print("into world, wait for 1s");
	co_await coro::Sleep(1s);
	print("world");
}

coro::Task<void> when_all_hello_world()
{
	auto [a, b] = co_await coro::when_all(hello(), world());
	print(a, b);
}

void when_all_test()
{
	asyncio.add(when_all_hello_world());
	asyncio.run();
}

coro::Task<void> when_any_hello_world()
{
	auto a = co_await coro::when_any(hello(), world());
}

void when_any_test()
{
	asyncio.add(when_any_hello_world());
	asyncio.run();
}

void sleep_sort_test()
{
	auto async_num = [](int num) -> Task<void>
		{
			co_await coro::Sleep(num * 1s);
			print(num);
			co_return;
		};

	asyncio.add(async_num(3));
	asyncio.add(async_num(2));
	asyncio.add(async_num(1));
	asyncio.add(async_num(5));
	asyncio.add(async_num(4));
	asyncio.run();
}