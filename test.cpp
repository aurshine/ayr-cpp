#include <law/Chain.hpp>
#include <law/Dict.hpp>
#include <law/timer.hpp>
#include <law/Dynarray.hpp>
#include <law/Iterator.hpp>
#include <law/AString.hpp>


using namespace ayr;

const int N = 1e7;

void t1()
{
	DynArray<int> a;
	for (int i = 0; i < N; i++)
		a.append(i);
}

void t2()
{
	std::vector<int> a;
	for (int i = 0; i < N; ++i)
		a.push_back(i);
}


int main()
{
	Timer timer("s");

	timer(t2);
	timer(t1);
	return 0;
}