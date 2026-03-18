#include <ayr/base.hpp>

using namespace ayr;

int main()
{
	auto f = []() {
		print("hello, world");
		};
	exitask(f);
	print("hello");
	return 0;
}