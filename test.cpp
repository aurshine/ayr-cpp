#include <vector>
#include <string>
#include <law/Array.hpp>
#include <law/DynArray.hpp>
#include <law/timer.hpp>

ayr::DynArray<std::string> das;
std::vector<std::string> vs;

constexpr int N = 100000;

void test_das()
{
	for (int i = 0; i < N; ++i)
	{
		das.append("hello");
	}
}

void test_vs()
{
	for (int i = 0; i < N; ++i)
	{
		vs.push_back("hello");
	}
}

int main()
{
	auto tm = ayr::Timer();

	tm(test_das);
	tm(test_vs);

	return 0;
}