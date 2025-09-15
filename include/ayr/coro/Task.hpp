#ifndef AYR_CORO_TASK_HPP
#define AYR_CORO_TASK_HPP

#include "Awaitable.hpp"

namespace ayr
{
	namespace coro
	{
		template<typename T>
		class Task : public Object<Task<T>>
		{
		public:
			using promise_type = Promise<T>;

			using co_type = std::coroutine_handle<promise_type>;

			friend class IoContext;
		private:
			using self = Task<T>;

			using super = Object<self>;

			co_type coro_;
		public:
			Task(co_type coro) : coro_(coro) {}

			Task(self&& other) noexcept : coro_(std::exchange(other.coro_, nullptr)) {}

			~Task() { if (coro_) coro_.destroy(); }

			self& operator=(self&& other) noexcept
			{
				if (this == &other) return *this;
				coro_ = std::exchange(other.coro_, nullptr);
				return *this;
			}

			// 当协程对象为空，或协程已经完成，则返回true
			bool await_ready() const noexcept { return coro_ == nullptr || coro_.done(); }

			co_type await_suspend(Coroutine coro) noexcept
			{
				coro_.promise().continuation = coro;
				return coro_;
			}

			T await_resume() const noexcept
			{
				if constexpr (!std::is_void_v<T>)
					return coro_.promise().result();
			}
		};
	}
}
#endif 