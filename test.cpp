#include <law/Chain.hpp>
#include <law/Dict.hpp>
#include <law/timer.hpp>
#include <law/Dynarray.hpp>

#include <unordered_map>
using namespace ayr;

const int N = 1e5;
Dict<int, int> d1;
std::unordered_map<int, int> d2;

void f1()
{	
	for (int i = 0; i < N; ++i)
		d1[i] = i;
}

void f2()
{
	for (int i = 0; i < N; ++i)
		d2[i] = 1;
}


int main()
{
	Timer timer("s");
	print(is_none(None<int>), is_none(1));

	/*timer(f2);
	timer(f1);*/
	
	
	return 0;
}