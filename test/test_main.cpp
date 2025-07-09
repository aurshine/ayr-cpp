#include <ayr/async/ThreadPool.hpp>
#include <ayr/Array.hpp>
#include <ayr/base/View.hpp>
#include <ayr/Dict.hpp>
#include <ayr/Atring.hpp>
#include <ayr/json.hpp>
#include <ayr/filesystem.hpp>
#include <ayr/Timer.hpp>
#include <ayr/net.hpp>

using namespace ayr;

int main()
{
	auto a1 = arr(dstr("呢哈哈嗲花"), dstr("发我忘记哦分环卫工"));
	auto a2 = a1;

	print(a1);
	print(a2);
	return 0;
}