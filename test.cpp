#include <law/Chain.hpp>
#include <law/Dict.hpp>
#include <law/timer.hpp>
#include <law/Dynarray.hpp>

#include <unordered_map>
using namespace ayr;

const int N = 1e7;
DynArray<int> arr;
std::vector<int> vec;

void f1()
{	
	for (int i = 0; i < N; ++i)
		arr.append(i);
	for (int i = 0; i < N; ++i)
		arr[i];
}

void f2()
{
	for (int i = 0; i < N; ++i)
		vec.push_back(i);
	for (int i = 0; i < N; ++i)
		vec[i];
}


int main()
{
	Timer timer("us");
	_BlockCache::get(0);

	timer(f1);
	timer(f2);
	
	return 0;
}