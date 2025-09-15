#include <ayr/async/ThreadPool.hpp>
#include <ayr/Array.hpp>
#include <ayr/base/View.hpp>
#include <ayr/Dict.hpp>
#include <ayr/Atring.hpp>
#include <ayr/json.hpp>
#include <ayr/filesystem.hpp>
#include <ayr/Timer.hpp>
#include <map>

using namespace ayr;

int main()
{
	std::map<int, int> mp;
	mp[1] = 2;
	mp[3] = 4;
	mp[5] = 6;

	print(mp.rbegin()->first);
	return 0;
}