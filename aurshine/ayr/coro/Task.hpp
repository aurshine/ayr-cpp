#ifndef AYR_CORO_TASK_HPP
#define AYR_CORO_TASK_HPP

#include "Promise.hpp"

namespace ayr
{
	namespace coro
	{
		template<typename T>
		class Task : public Object<Task<T>>
		{
			using self = Task<T>;
		public:
			using promise_type = Promise<T>;

			using co_type = promise_type::co_type;

			Task(co_type coroutine) : coro_(coroutine) {}

			Task(Task&& other) noexcept : coro_(other.coro_) { other.coro_ = nullptr; }

			Task(const Task& other) = delete;

			~Task()
			{
				if (coro_)
				{
					coro_.destroy();
					coro_ = nullptr;
				}
			}

			Task& operator=(const Task& other) = delete;

			Task& operator=(Task&& other) noexcept
			{
				if (this != &other)
				{
					coro_ = other.coro_;
					other.coro_ = nullptr;
				}
				return *this;
			}

			operator co_type() const noexcept { return coro_; }

			bool await_ready() const noexcept { return false; }

			co_type await_suspend(Coroutine awaiting_coro) noexcept { return coro_; }

			T await_resume() const { return coro_.promise().result(); }

			const co_type& coroutine() const noexcept { return coro_; }
		private:
			co_type coro_;
		};

		template<>
		class Task<void> : public Object<Task<void>>
		{
			using self = Task<void>;
		public:
			using promise_type = Promise<void>;

			using co_type = promise_type::co_type;

			Task(co_type coroutine) : coro_(coroutine) {}

			Task(Task&& other) noexcept : coro_(other.coro_) { other.coro_ = nullptr; }

			Task(const Task& other) = delete;

			~Task()
			{
				if (coro_)
				{
					coro_.destroy();
					coro_ = nullptr;
				}
			}

			Task& operator=(const Task& other) = delete;

			Task& operator=(Task&& other) noexcept
			{
				if (this != &other)
				{
					coro_ = other.coro_;
					other.coro_ = nullptr;
				}
				return *this;
			}

			operator co_type() const noexcept { return coro_; }

			bool await_ready() const noexcept { return false; }

			co_type await_suspend(Coroutine awaiting_coro) noexcept { return coro_; }

			void await_resume() const {}

			const co_type& coroutine() const noexcept { return coro_; }
		private:
			co_type coro_;
		};
	}
}
#endif 