#include <ayr/async/ThreadPool.hpp>
#include <ayr/Array.hpp>
#include <ayr/base/View.hpp>
#include <ayr/Dict.hpp>
#include <ayr/Atring.hpp>
#include <ayr/json.hpp>
#include <ayr/filesystem.hpp>
#include <ayr/Timer.hpp>

using namespace ayr;


int main()
{
	Array<int> arr = { 1, 2, 3, 4, 5 };
	print(arr);
	return 0;
}