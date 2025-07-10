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

struct GenTest
{
	GenTest() { print("GenTest()"); }

	//GenTest(GenTest&) { print("GenTest(GenTest&)"); }

	GenTest(GenTest&&) { print("GenTest(GenTest&&)"); }

	~GenTest() { print("~GenTest()"); }

	//GenTest& operator=(GenTest&) { print("GenTest& operator=(GenTest&)"); }

	GenTest& operator=(GenTest&&) { print("GenTest& operator=(GenTest&&)"); }
};

coro::Generator<GenTest> gen_test()
{
	print("--- gen_test() begin");
	for (int i = 0; i < 2; i++)
	{
		print("+++ gen_test() yield");
		co_yield GenTest();
		print("*** gen_test() yield end");
	}
}

coro::Generator<int> no_yield()
{
	if (false) co_yield 1;
}

coro::Generator<int> only_return()
{
	co_return 1;
}

int main()
{
	/*for (int i : numbers(10))
		print(i);

	for (auto obj : no_default_constructor())
		print(obj);*/

	auto gen = gen_test();
	print("gen");
	for (auto& obj : gen)
	{
		print("loop\n");
	}

	for (auto& i : no_yield())
		print(i);
	for (auto& i : only_return())
		print(i);

	return 0;
}