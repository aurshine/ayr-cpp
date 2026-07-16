#include <atomic>
#include <mutex>
#include <vector>

#include <ayr/async.hpp>

#include <ayr/base/utest.hpp>

using namespace ayr;

int main()
{
	// 测试 then 构成的依赖图：子任务必须等父任务执行后才能运行。
	async::AsyncExecutor exec(3);
	std::atomic<bool> task1_done = false;
	std::atomic<bool> task2_done = false;
	std::atomic<bool> task3_done = false;
	std::atomic<bool> task4_done = false;
	std::atomic<int> completed = 0;

	auto task1 = exec.create_task([&] {
		task1_done = true;
		++completed;
	});
	auto task2 = exec.create_task([&] {
		AYR_TEST_EXPECT(task1_done.load());
		task2_done = true;
		++completed;
	});
	auto task3 = exec.create_task([&] {
		AYR_TEST_EXPECT(task1_done.load());
		task3_done = true;
		++completed;
	});
	auto task4 = exec.create_task([&] {
		AYR_TEST_EXPECT(task2_done.load());
		AYR_TEST_EXPECT(task3_done.load());
		task4_done = true;
		++completed;
	});
	auto task5 = exec.create_task([&] {
		AYR_TEST_EXPECT(task4_done.load());
		++completed;
	});

	task1->then(task2);
	task1->then(task3);
	task2->then(task4);
	task3->then(task4);
	task4->then(task5);
	exec.run();
	AYR_TEST_EXPECT_EQ(completed.load(), 5);

	// 测试带返回值任务能把 future 结果传回调用方。
	async::AsyncExecutor ret_exec(2);
	auto [ret_task, future] = ret_exec.create_ret_task([] { return 40 + 2; });
	(void)ret_task;
	ret_exec.run();
	AYR_TEST_EXPECT_EQ(future.get(), 42);

	// 测试 clear 会移除尚未运行的任务。
	async::AsyncExecutor clear_exec(1);
	std::atomic<bool> ran = false;
	clear_exec.create_task([&] { ran = true; });
	clear_exec.clear();
	clear_exec.run();
	AYR_TEST_EXPECT(!ran.load());
}
