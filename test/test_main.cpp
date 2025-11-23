#include <ayr/base/itertools.hpp>
#include <ayr/base/raise_error.hpp>

using namespace ayr;

struct A
{
	A() { print("A()"); }

	A(const A&) { print("A(const A&)"); }

	A(A&&) { print("A(A&&)"); }

	~A() { print("~A()"); }
};

void f(const A& a)
{
	print("f(const A&)");
}

A a() { return A(); }

int main()
{
	f(a());

	return 0;
}