#include <vector>
#include <thread/Lock.hpp>
#include "law/Dynarray.hpp"
#include "law/timer.hpp"
#include "law/implemented.hpp"
#include "law/Array.hpp"
#include "law/String.hpp"

using namespace ayr;
int N = 1e2;
DynArray<std::string> da;
std::vector<std::string> v;

int cnt;
Lock lock;

void f()
{
	lock.lock();
	for (int i = 0; i < 1000; ++i)
		cnt++;
	lock.unlock();
}

void work()
{
	const int N = 25;
	std::thread t[N];
	for (int i = 0; i < N; ++i)
		t[i] = std::thread(f);

	for (int i = 0; i < N; ++i)
		t[i].join();

	std::cout << cnt << std::endl;
}

int main()
{
	Timer t;
	t(work);
}