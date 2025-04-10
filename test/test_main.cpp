#include <ayr/async/ThreadPool.hpp>
#include <ayr/Array.hpp>
#include <ayr/base/View.hpp>
#include <ayr/Dict.hpp>
#include <ayr/Atring.hpp>

using namespace ayr;

int main()
{
	const Dict<Atring, Atring> d1 = { { "a", "1" }, { "b", "2" }, { "c", "3" } };

	for (auto& [key, value] : d1.items())
	{
		print("\n"as.ajoin(Array<View>({ key, value })));
	}
}