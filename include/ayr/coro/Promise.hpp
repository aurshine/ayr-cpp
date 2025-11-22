#ifndef AYR_CORO_PROMISE_HPP_
#define AYR_CORO_PROMISE_HPP_

#include "co_utils.hpp"
#include "../Optional.hpp"


namespace ayr
{
	namespace coro
	{
		template<typename T>
		struct PromiseImpl : public Object<PromiseImpl<T>>
		{
			using self = PromiseImpl<T>;

			using co_type = std::coroutine_handle<self>;

			std::suspend_always initial_suspend() const noexcept { return {}; }

			std::suspend_always final_suspend() const noexcept { return {}; }

			std::suspend_always yield_value(T value) noexcept { value_ = std::move(value); return {}; }

			void unhandled_exception() { throw; }

			void return_value(T value) noexcept { value_ = std::move(value); }

			co_type get_return_object() noexcept { return co_type::from_promise(*this); }

			T result() noexcept
			{
				if (!value_.has_value())
					RuntimeError("promise witout co_return or co_yield value");
				return std::move(*value_);
			}

			Optional<T> value_;
		};

		template<>
		struct PromiseImpl<void> : public Object<PromiseImpl<void>>
		{
			using self = PromiseImpl<void>;

			using co_type = std::coroutine_handle<self>;

			std::suspend_always initial_suspend() const noexcept { return {}; }

			std::suspend_always final_suspend() const noexcept { return {}; }

			std::suspend_always yield_value() noexcept { return {}; }

			void unhandled_exception() { throw; }

			void return_void() noexcept {}

			co_type get_return_object() noexcept { return co_type::from_promise(*this); }
		};

		template<typename T = void>
		struct Promise : public PromiseImpl<T>
		{
			using self = Promise<T>;

			using super = PromiseImpl<T>;

			using co_type = std::coroutine_handle<self>;

			/*
			* @brief 最终挂起的协程
			*
			* @details 如果continuation 不为空，则恢复continuation
			*/
			CoroAwaiter final_suspend() const noexcept { return CoroAwaiter(continuation); }

			co_type get_return_object() noexcept { return co_type::from_promise(*this); }

			Coroutine continuation = nullptr;
		};
	}
}
#endif // AYR_CORO_PROMISE_HPP_