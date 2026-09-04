#include <chrono>

#include <ayr/coro.hpp>
#include <ayr/base/utest.hpp>

using namespace ayr;

namespace
{
	struct FrameLifetime
	{
		static inline int alive = 0;

		FrameLifetime() { ++alive; }
		FrameLifetime(const FrameLifetime&) { ++alive; }
		FrameLifetime(FrameLifetime&&) noexcept { ++alive; }
		~FrameLifetime() { --alive; }
	};

	coro::Task<int> immediate_value(int value)
	{
		co_return value;
	}

	coro::Task<int> nested_value(int value)
	{
		int result = co_await immediate_value(value);
		co_return result + 1;
	}

	coro::Task<void> increment(int* value)
	{
		++*value;
		co_return;
	}

	coro::Task<int> delayed_value(coro::IoContext* io_context, int value)
	{
		co_await coro::Sleep(std::chrono::milliseconds(1), io_context);
		co_return value;
	}

	coro::Task<bool> captures_current_coroutine()
	{
		coro::Coroutine current = co_await coro::CurrentCoro();
		co_return current != nullptr;
	}

	coro::Task<int> failing_task()
	{
		RuntimeError("task failure");
		co_return 0;
	}

	coro::Task<int> nested_failing_task()
	{
		co_return co_await failing_task();
	}

	coro::Task<int> missing_result_task()
	{
		co_await std::suspend_never{};
	}

	coro::Task<void> owns_frame(FrameLifetime lifetime)
	{
		(void)lifetime;
		co_return;
	}
}

int main()
{
	UTEST_SCOPE("测试 IoContext 执行立即返回、嵌套等待和 void Task。")
	{
		coro::IoContext io_context;
		auto direct = immediate_value(41);
		UTEST_EXPECT(!direct.await_ready());
		UTEST_EXPECT_EQ(io_context.run(direct), 41);
		UTEST_EXPECT(direct.await_ready());

		auto nested = nested_value(41);
		UTEST_EXPECT_EQ(io_context.run(nested), 42);

		int value = 0;
		auto no_result = increment(&value);
		io_context.run(no_result);
		UTEST_EXPECT_EQ(value, 1);
		UTEST_EXPECT(io_context.empty());
	};

	UTEST_SCOPE("测试 Sleep 定时恢复及 CurrentCoro 返回当前协程。")
	{
		coro::IoContext io_context;
		auto delayed = delayed_value(&io_context, 9);
		auto start = std::chrono::steady_clock::now();
		UTEST_EXPECT_EQ(io_context.run(delayed), 9);
		auto elapsed = std::chrono::steady_clock::now() - start;
		UTEST_EXPECT(elapsed >= std::chrono::milliseconds(1));

		auto current = captures_current_coroutine();
		UTEST_EXPECT(io_context.run(current));
	};

	UTEST_SCOPE("测试 Task 移动构造和移动赋值保持协程所有权。")
	{
		coro::IoContext io_context;
		auto source = immediate_value(7);
		auto moved = std::move(source);
		UTEST_EXPECT(source.await_ready());
		UTEST_EXPECT_EQ(io_context.run(moved), 7);

		auto other = immediate_value(8);
		auto replacement = immediate_value(9);
		other = std::move(replacement);
		UTEST_EXPECT(replacement.await_ready());
		UTEST_EXPECT_EQ(io_context.run(other), 9);
		other = std::move(other);
		UTEST_EXPECT_EQ(other.await_resume(), 9);
	};

	UTEST_SCOPE("测试 Task 移动赋值会释放原有协程帧。")
	{
		FrameLifetime::alive = 0;
		{
			auto first = owns_frame(FrameLifetime{});
			auto second = owns_frame(FrameLifetime{});
			UTEST_EXPECT_EQ(FrameLifetime::alive, 2);
			first = std::move(second);
			UTEST_EXPECT_EQ(FrameLifetime::alive, 1);
			UTEST_EXPECT(second.await_ready());
		}
		UTEST_EXPECT_EQ(FrameLifetime::alive, 0);
	};

	UTEST_SCOPE("测试 Promise 保存异常和缺失返回值错误。")
	{
		coro::IoContext io_context;
		auto failing = failing_task();
		UTEST_EXPECT_AYR_ERROR(io_context.run(failing));
		auto nested_failing = nested_failing_task();
		UTEST_EXPECT_AYR_ERROR(io_context.run(nested_failing));

		auto missing = missing_result_task();
		UTEST_EXPECT_AYR_ERROR(io_context.run(missing));
	};

	UTEST_SCOPE("测试无效描述符的事件 awaiter 会立即完成。")
	{
		Buffer buffer;
		coro::ReadWaiter read_waiter(-1, &buffer, nullptr);
		coro::WriteWaiter write_waiter(-1, &buffer, nullptr);
		coro::AcceptWaiter accept_waiter(-1, AF_INET, nullptr);
		coro::ConnectWaiter connect_waiter(-1, nullptr, nullptr);
		UTEST_EXPECT(read_waiter.await_ready());
		UTEST_EXPECT(write_waiter.await_ready());
		UTEST_EXPECT(accept_waiter.await_ready());
		UTEST_EXPECT(connect_waiter.await_ready());
		UTEST_EXPECT(read_waiter.await_resume().ok());
		UTEST_EXPECT(write_waiter.await_resume().ok());
		UTEST_EXPECT(accept_waiter.await_resume().ok());
		UTEST_EXPECT(connect_waiter.await_resume().ok());
	};

	return UTEST_COMPLETE();
}
