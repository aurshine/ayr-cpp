#ifndef AYR_CORO_TASK_HPP
#define AYR_CORO_TASK_HPP

#include "Awaiter.hpp"

namespace ayr
{
	namespace coro
	{
		template<typename T>
		class Task : public Awaiter<T, Promise<T>>
		{
			using self = Task<T>;

			using super = Awaiter<T, Promise<T>>;
		public:
			friend class CoroLoop;

			using promise_type = super::promise_type;

			using co_type = super::co_type;

			Task() : super() {}

			Task(co_type coroutine) : super(coroutine) {}

			Task(Task&& other) noexcept : super(std::move(other)) {}

			Task& operator=(Task&& other) noexcept
			{
				super::operator=(std::move(other));
				return *this;
			}

			Coroutine await_suspend(Coroutine awaiter) noexcept
			{
				super::coro_.promise().previous_coro_ = awaiter;
				return super::coro_;
			}

			operator Coroutine() const noexcept { return super::coro_; }
		};
	}
}
#endif 