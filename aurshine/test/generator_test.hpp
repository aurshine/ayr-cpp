#include "ayr/coro.hpp"

using namespace ayr;

coro::Generator<int> numbers(int n)
{
	for (int i = n; i; i--)
	{
		if (i == 1)
			co_return n;
		co_yield i;
	}
}

void generator_test()
{
	for (int i : numbers(10))
		print(i);
}