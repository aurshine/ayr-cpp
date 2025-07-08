#include "ayr/coro.hpp"

using namespace ayr;

coro::Generator<int> numbers(int n)
{
	for (int i = n; i; i--)
		co_yield i;
	co_return n + 1;
}

struct NoDefaultConstructor : public Object<NoDefaultConstructor>
{
	NoDefaultConstructor(int x) : x(x) {}

	NoDefaultConstructor(const NoDefaultConstructor& other) : x(other.x) {}

	void __repr__(Buffer& buffer) const
	{
		buffer << "NoDefaultConstructor(" << x << ")";
	}

	int x;
};

coro::Generator<NoDefaultConstructor> no_default_constructor()
{
	for (int i = 0; i < 10; i++)
	{
		NoDefaultConstructor obj(i);
		co_yield std::move(obj);
	}

}

int main()
{
	for (int i : numbers(10))
		print(i);

	for (auto& obj : no_default_constructor())
		print(obj);
}