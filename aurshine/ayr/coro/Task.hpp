#ifndef AYR_CORO_TASK_HPP
#define AYR_CORO_TASK_HPP

#include "Awaiter.hpp"

namespace ayr
{
	namespace coro
	{
		template<typename T>
		class Task : public Awaiter<T>
		{
			using self = Task<T>;

			using super = Awaiter<T>;
		public:
			using promise_type = super::promise_type;

			using co_type = super::co_type;

			Task(co_type coroutine) : super(coroutine) {}

			Task(Task&& other) noexcept : super(std::move(other)) {}

			~Task() { if (super::coro_) { super::coro_.destroy(); super::coro_ = nullptr; } };

			operator co_type() const noexcept { return super::coro_; }
		};
	}
}
#endif 