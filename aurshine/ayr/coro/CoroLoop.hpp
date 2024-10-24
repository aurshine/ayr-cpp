#ifndef AYR_CORO_EVENTLOOP_HPP
#define AYR_CORO_EVENTLOOP_HPP

#include <queue>
#include <chrono>

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


		class CoroLoop : Object<CoroLoop>
		{
			using self = CoroLoop;

			CoroLoop() = default;
		public:
			CoroLoop(const self&) = delete;

			CoroLoop(self&&) = delete;

			self& operator=(const self&) = delete;

			self& operator=(self&&) = delete;

			void add_coro(Coroutine coro) { ready_coros.push_back(coro); }

			void add_coro(Coroutine coro, const std::chrono::steady_clock::time_point& abs_time)
			{
				sleep_coros.push({ coro, abs_time });
			}

			template<typename TaskType>
			void add_task(TaskType&& task) { add_coro(task.coro_); task.coro_ = nullptr; }

			void run()
			{
				while (!ready_coros.empty() || !sleep_coros.empty())
				{
					while (!ready_coros.empty())
					{
						ready_coros.front().resume();
						ready_coros.pop_front();
					}

					if (!sleep_coros.empty())
					{
						auto& first_timer = sleep_coros.top();

						if (first_timer.abs_time_ <= std::chrono::steady_clock::now())
							first_timer.coro_.resume();
						else
						{
							std::this_thread::sleep_until(first_timer.abs_time_);
							first_timer.coro_.resume();
						}

						sleep_coros.pop();
					}
				}
			}

			static self& get_loop()
			{
				static self coro_loop;
				return coro_loop;
			}

			template<typename T = void>
			static T& async_run(std::coroutine_handle<Promise<T>> coroutine)
			{
				while (!coroutine.done())
					coroutine.resume();

				if constexpr (std::is_void_v<T>)
					return;
				else
					return coroutine.promise().result();
			}

		private:
			std::deque<Coroutine> ready_coros;

			std::priority_queue<TimerEntry> sleep_coros;
		};

		class Sleep : public std::suspend_always, public Object<Sleep>
		{
		public:
			Sleep(const std::chrono::steady_clock::time_point& abs_time)
				: abs_time_(abs_time) {}

			Sleep(const std::chrono::steady_clock::duration& rel_time)
				: abs_time_(std::chrono::steady_clock::now() + rel_time) {}

			void await_suspend(Coroutine coroutine)
			{
				CoroLoop::get_loop().add_coro(coroutine, abs_time_);
			}

		private:
			std::chrono::steady_clock::time_point abs_time_;
		};
	}
}
#endif