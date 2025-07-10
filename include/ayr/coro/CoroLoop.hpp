#ifndef AYR_CORO_EVENTLOOP_HPP
#define AYR_CORO_EVENTLOOP_HPP

#include <queue>
#include <chrono>
#include <thread>

#include "Task.hpp"

namespace ayr
{
	namespace coro
	{
		struct TimerEntry
		{
			TimerEntry(Coroutine coro, const std::chrono::steady_clock::time_point& abs_time)
				: coro_(coro), abs_time_(abs_time) {}

			bool operator< (const TimerEntry& other) const { return abs_time_ > other.abs_time_; }

			Coroutine coro_;

			std::chrono::steady_clock::time_point abs_time_;
		};


		class CoroLoop : public Object<CoroLoop>
		{
			using self = CoroLoop;
		public:
			CoroLoop() = default;

			// 恢复一个协程
			void resume(Coroutine coro) { coro.resume(); }

			// 添加一个协程到Loop中, 返回协程的promise
			template<typename P>
			void_or_ref_t<P> add(std::coroutine_handle<P> coro)
			{
				ready_coroutines.push_back(coro);
				if constexpr (std::is_void_v<P>)
					return coro;
				else
					return coro.promise();
			}

			// 添加一个协程到Loop中, 返回协程的promise, 并设置定时器
			template<typename P>
			void_or_ref_t<P> add(std::coroutine_handle<P> coro, const std::chrono::steady_clock::time_point& abs_time)
			{
				sleep_coroutines.push({ coro, abs_time });
				if constexpr (std::is_void_v<P>)
					return;
				else
					return coro.promise();
			}

			// 添加一个Task到Loop中, 返回Task的promise
			template<typename T>
			decltype(auto) add(Task<T>& task)
			{
				typename Task<T>::co_type coro = task.coro_;
				task.coro_ = nullptr;

				return add(coro);
			}

			template<typename T>
			decltype(auto) add(Task<T>&& task) { return add(task); }

			// 运行Loop
			void run()
			{
				while (!ready_coroutines.empty() || !sleep_coroutines.empty())
				{
					while (!ready_coroutines.empty())
					{
						resume(ready_coroutines.front());
						ready_coroutines.pop_front();
					}

					if (!sleep_coroutines.empty())
					{
						auto& first_timer = sleep_coroutines.top();

						if (first_timer.abs_time_ > std::chrono::steady_clock::now())
							std::this_thread::sleep_until(first_timer.abs_time_);

						resume(first_timer.coro_);
						sleep_coroutines.pop();
					}
				}
			}
		private:
			std::deque<Coroutine> ready_coroutines;

			std::priority_queue<TimerEntry> sleep_coroutines;
		};

		static CoroLoop asyncio{};

		class Sleep : public std::suspend_always, public Object<Sleep>
		{
		public:
			Sleep(const std::chrono::steady_clock::time_point& abs_time)
				: abs_time_(abs_time) {}

			Sleep(const std::chrono::steady_clock::duration& rel_time)
				: abs_time_(std::chrono::steady_clock::now() + rel_time) {}

			void await_suspend(std::coroutine_handle<void> coroutine)
			{
				asyncio.add<void>(coroutine, abs_time_);
			}
		private:
			std::chrono::steady_clock::time_point abs_time_;
		};
	}
}
#endif