#include <law/Chain.hpp>
#include <law/Dict.hpp>
#include <law/timer.hpp>
#include <law/Dynarray.hpp>

#include <unordered_map>
using namespace ayr;

const int N = 1e7;

void f1()
{
	for (auto&& i : Range(1, N, 2))
		i;
}

void f2()
{
	for (int i = 0; i < N; i += 2)
		i;
}


int main()
{
	Timer timer("ms");

	timer(f2);
	timer(f1);


	return 0;
}