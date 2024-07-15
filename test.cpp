#include <law/Chain.hpp>
#include <law/Dict.hpp>
#include <law/timer.hpp>
#include <law/Dynarray.hpp>

#include <unordered_map>
using namespace ayr;

const int N = 1e6;
Dict<CString, int> d1;
std::unordered_map<std::string, int> d2;

void f1()
{
	for (int i = 0; i < N; ++i)
		d1.setkv2bucket(d1.mk_kv(std::format("key{}", i).c_str(), i));
}

void f2()
{
	for (int i = 0; i < N; ++i)
		d2[std::format("key{}", i)] = i;
}


int main()
{
	Timer timer("s");

	timer(f2);
	timer(f1);


	return 0;
}