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
	t1->add_child(t2);
	t1->add_child(t3);
	t2->add_child(t4);
	t3->add_child(t4);
	t4->add_child(t5);
	
	exec.run();
}