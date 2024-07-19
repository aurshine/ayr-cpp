#include <law/Chain.hpp>
#include <law/Dict.hpp>
#include <law/timer.hpp>
#include <law/Dynarray.hpp>
#include <law/Iterator.hpp>
#include <law/AString.hpp>


using namespace ayr;

const int N = 1e7;


int main()
{
	Timer timer("ms");

	/*timer(f2);
	timer(f1);*/

	const Dict<int, int> d{ {1, 2}, {3, 4} };

	for (auto&& kv : d)
		print(kv.key, kv.value);
	return 0;
}