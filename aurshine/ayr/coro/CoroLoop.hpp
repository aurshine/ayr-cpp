#ifndef AYR_CORO_EVENTLOOP_HPP
#define AYR_CORO_EVENTLOOP_HPP

#include <queue>
#include <chrono>

#include "Task.hpp"
#include <ayr/detail/ayr_traits.hpp>

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


		class CoroLoop : public Object<CoroLoop>, public NoCopy
		{
			using self = CoroLoop;

			CoroLoop() = default;
		public:
			static self& get_loop() { static self coro_loop; return coro_loop; }

			template<typename P>
			static void_or_ref_t<P> add(std::coroutine_handle<P> coro)
			{
				get_loop().ready_coroutines.push_back(coro);
				if constexpr (std::is_void_v<P>)
					return;
				else
					return coro.promise();
			}

			template<typename P>
			static void_or_ref_t<P> add(std::coroutine_handle<P> coro, const std::chrono::steady_clock::time_point& abs_time)
			{
				get_loop().sleep_coroutines.push({ coro, abs_time });
				if constexpr (std::is_void_v<P>)
					return;
				else
					return coro.promise();
			}

			template<typename T>
			static decltype(auto) add(Task<T>& task)
			{
				typename Task<T>::co_type coro = task.coro_;
				task.coro_ = nullptr;

				return add(coro);
			}

			template<typename T>
			static decltype(auto) add(Task<T>&& task)
			{
				return add(task);
			}

			static void run()
			{
				while (!get_loop().ready_coroutines.empty() || !get_loop().sleep_coroutines.empty())
				{
					while (!get_loop().ready_coroutines.empty())
					{
						get_loop().ready_coroutines.front().resume();
						get_loop().ready_coroutines.pop_front();
					}

					if (!get_loop().sleep_coroutines.empty())
					{
						auto& first_timer = get_loop().sleep_coroutines.top();

						if (first_timer.abs_time_ <= std::chrono::steady_clock::now())
							first_timer.coro_.resume();
						else
						{
							std::this_thread::sleep_until(first_timer.abs_time_);
							first_timer.coro_.resume();
						}

						get_loop().sleep_coroutines.pop();
					}
				}
			}
		private:
			std::deque<Coroutine> ready_coroutines;

			std::priority_queue<TimerEntry> sleep_coroutines;
		};


		class Sleep : public std::suspend_always, public Object<Sleep>
		{
		public:
			Sleep(const std::chrono::steady_clock::time_point& abs_time)
				: abs_time_(abs_time) {}

			Sleep(const std::chrono::steady_clock::duration& rel_time)
				: abs_time_(std::chrono::steady_clock::now() + rel_time) {}

			void await_suspend(std::coroutine_handle<void> coroutine)
			{
				CoroLoop::add<void>(coroutine, abs_time_);
			}

		private:
			std::chrono::steady_clock::time_point abs_time_;
		};
	}
}
#endif