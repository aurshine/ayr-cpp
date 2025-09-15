#ifndef AYR_CORO_EVENTLOOP_HPP
#define AYR_CORO_EVENTLOOP_HPP

#include <chrono>
#include <map>
#include <queue>
#include <thread>

#include "EventAwaiter.hpp"

namespace ayr
{
	namespace coro
	{
		struct TimerEntry
		{
			TimerEntry(Coroutine coro, const std::chrono::steady_clock::time_point& abs_time)
				: coro_(coro), abs_time_(abs_time) {
			}

			bool operator< (const TimerEntry& other) const { return abs_time_ > other.abs_time_; }

			Coroutine coro_;

			std::chrono::steady_clock::time_point abs_time_;
		};

		/*
		* @brief 协程事件循环
		*
		* @detail 可以使用add向事件循环里添加协程句柄，事件循环不会控制协程句柄的生命周期。
		*/
		class IoContext : public Object<IoContext>
		{
			using self = IoContext;

			using super = Object<self>;

			using TimePoint = std::chrono::steady_clock::time_point;

			// 就绪协程队列
			std::queue<Coroutine> ready_coros_;

			// 等待协程队列
			std::multimap<TimePoint, Coroutine> wait_coros_;

			net::IoWaiter io_waiter_;
		public:
			IoContext() {}

			/*
			* @brief 添加一个协程到Loop中, 返回协程的promise
			* 如果协程的promise类型为void, 则返回void
			*
			* @tparam P 协程的promise类型
			*
			* @param coro 协程句柄
			*
			* @return 协程的promise
			*/
			template<typename P>
			void_or_ref_t<P> add(std::coroutine_handle<P> coro)
			{
				ready_coros_.push(coro);
				if constexpr (!std::is_void_v<P>)
					return coro.promise();
			}

			/*
			* @brief 添加一个等待协程到Loop中, 返回协程的promise
			* 如果协程的promise类型为void, 则返回void
			*
			* @tparam P 协程的promise类型
			*
			* @param coro 协程句柄
			*
			* @param abs_time 协程被唤醒的绝对时间点
			*
			* @return 协程的promise
			*/
			template<typename P>
			void_or_ref_t<P> add(std::coroutine_handle<P> coro, const std::chrono::steady_clock::time_point& abs_time)
			{
				wait_coros_.insert({ abs_time, coro });
				if constexpr (!std::is_void_v<P>)
					return coro.promise();
			}

			// 事件循环是否为空
			bool empty() const { return ready_coros_.empty() && wait_coros_.empty() && io_waiter_.empty(); }

			// 启动事件循环，直到所有协程都执行完毕
			void run()
			{
				while (!empty())
				{
					// 处理等待完成的协程
					while (!wait_coros_.empty())
					{
						auto& [time_point, coro] = *wait_coros_.begin();

						if (time_point <= std::chrono::steady_clock::now())
							ready_coros_.push(coro);
						else
							break;
						wait_coros_.erase(wait_coros_.begin());
					}

					// 处理已经准备好的协程
					while (!ready_coros_.empty())
					{
						ready_coros_.front().resume();
						ready_coros_.pop();
					}

					int timeout_ms = ifelse(io_waiter_.empty(), 0, -1);
					// 计算超时时间
					if (!wait_coros_.empty())
					{
						auto& [time_point, coro] = *wait_coros_.begin();
						timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_point - std::chrono::steady_clock::now()).count();
						timeout_ms = std::max(timeout_ms, 0);
					}

					if (io_waiter_.empty())
						std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
					else
					{
						auto io_events = io_waiter_.wait(timeout_ms);
						for (auto& io_event : io_events)
							ready_coros_.push(io_event.data);
					}
				}
			}

			template<typename T>
			T run(const Task<T>& task)
			{
				if constexpr (std::is_void_v<T>)
				{
					add(task.coro_);
					run();
				}
				else
				{
					Promise<T>& p = add(task.coro_);
					run();
					return p.result();
				}
			}

			// 构造读等待对象
			EventAwaiter wait_for_read(int fd) { return EventAwaiter(fd, net::IoEvent::READABLE, &io_waiter_); }

			// 构造写等待对象
			EventAwaiter wait_for_write(int fd) { return EventAwaiter(fd, net::IoEvent::WRITABLE, &io_waiter_); }
		};

		static IoContext asyncio = IoContext();

		/*
		* @brief 等待时间的协程等代体
		*/
		class Sleep : public Object<Sleep>
		{
		public:
			Sleep(const std::chrono::steady_clock::time_point& abs_time, IoContext* io_context = nullptr) :
				io_context_(io_context),
				abs_time_(abs_time)
			{
				if (io_context == nullptr)
					io_context_ = &asyncio;
			}

			Sleep(const std::chrono::steady_clock::duration& rel_time, IoContext* io_context = nullptr) :
				Sleep(std::chrono::steady_clock::now() + rel_time, io_context) {
			}

			bool await_ready() const { return false; }

			void await_suspend(std::coroutine_handle<void> coroutine)
			{
				io_context_->add<void>(coroutine, abs_time_);
			}

			void await_resume() const {}
		private:
			std::chrono::steady_clock::time_point abs_time_;

			IoContext* io_context_;
		};
	}
}
#endif