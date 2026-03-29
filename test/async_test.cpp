#include <ayr/async.hpp>

using namespace ayr;
using namespace std::literals;

def task(int i)
{
	print("Starting task ", i);
	std::this_thread::sleep_for(1s);
	print("Finished task ", i);
}

int main()
{
	async::AsyncExecutor exec(3);
	auto t1 = exec.create_task([] { task(1); });
	auto t2 = exec.create_task([] { task(2); });
	auto t3 = exec.create_task([] { task(3); });
	auto t4 = exec.create_task([] { task(4); });
	auto t5 = exec.create_task([] { task(5); });
	t1->then(t2);
	t1->then(t3);
	t2->then(t4);
	t3->then(t4);
	t4->then(t5);
	exec.run();
}