#ifndef AYR_CORO_TASK_HPP
#define AYR_CORO_TASK_HPP

#include "Promise.hpp"

namespace ayr
{
	namespace coro
	{
		template<typename T>
		class Task : public Object<T>
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

			operator std::coroutine_handle<>() const noexcept { return coro_; }

			bool await_ready() const noexcept { return false; }

			coroutine await_suspend(coroutine awaiting_coro) noexcept { return coro_; }

			T await_resume() const { return coro_.promise().result(); }
		private:
			co_type coro_;
		};

		def async_run(const coroutine& coroutine) { return coroutine.resume(); }
	}
}
#endif 