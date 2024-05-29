#include "law/implemented.hpp"
#include "law/Array.hpp"
#include "law/String.hpp"
#include "law/Dynarray.hpp"
#include <vector>


int main()
{
	using namespace ayr;
	int N = 1000000;
	DynArray<int> da;
	std::vector<int> v;

	auto start = clock();
	for (int i = 0; i < N; ++i)
		da.append(i);

	print(clock() - start);

	start = clock();
	for (int i = 0; i < N; ++i)
		v.push_back(i);

	print(clock() - start);
}