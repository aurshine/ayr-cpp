#define AYR_DEBUG

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

	CString s1 = "hello world", s2 = "world";
	Dict<CString, int> d3;
	d3[s1] = 1;
	d3[s2] = 2;
	print(d3[s1], d3[s2]);
	d3[s1] = 3;
	print(d3[s1], d3[s2]);
	int v = 10;
	print(d3.get(CString("hello worl")));

	/*timer(f2);
	timer(f1);*/


	return 0;
}