#include <ayr/Chain.hpp>
#include <ayr/Dict.hpp>
#include <ayr/timer.hpp>
#include <ayr/DynArray.hpp>
#include <ayr/Atring.hpp>
#include <ayr/timer.hpp>
#include <ayr/log.hpp>
#include <ayr/fs/filesystem.hpp>
#include <ayr/Set.hpp>
#include <ayr/json/json.hpp>
#include <ayr/coro/coro.hpp>
#include <ayr/thread/ThreadPool.hpp>

#include <ayr/socket/Socket.hpp>

using namespace ayr;
using namespace std::chrono_literals;

int main()
{

	A* a = (A*)operator new(sizeof(A) * 10);
	for (int i = 0; i < 10; i++)
		::new(a + i) A();

	delete[] a;
	return 0;
}