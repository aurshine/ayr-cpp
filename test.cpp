#include <law/Chain.hpp>
#include <law/Dict.hpp>
#include <law/timer.hpp>
#include <law/Dynarray.hpp>

#include <unordered_map>
using namespace ayr;

void f1(Timer& timer)
{
	Dict<std::string, int> dict;
	for (int i = 0; i < 10000; ++i)
		dict[std::format("key{}", i)] = i;
	for (int i = 0; i < 10000; ++i)
		dict[std::format("key{}", i)];
}

void f2()
{
	std::unordered_map<std::string, int> map;
	for (int i = 0; i < 10000; ++i)
		map[std::format("key{}", i)] = i;
	for (int i = 0; i < 10000; ++i)
		map[std::format("key{}", i)];

}


int main()
{
	Timer timer("ms");

	timer(f1, timer);
	timer(f2);
	return 0;
}