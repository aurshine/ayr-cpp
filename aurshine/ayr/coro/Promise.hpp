#ifndef AYR_CORO_PROMISE_HPP_
#define AYR_CORO_PROMISE_HPP_

#include <coroutine>

#include <ayr/detail/printer.hpp>
#include <ayr/detail/NoCopy.hpp>
#include "co_utils.hpp"


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

			T& result() noexcept { return value_; }

			const T& result() const noexcept { return value_; }

			T value_;
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

			CoroAwaiter final_suspend() const noexcept { return CoroAwaiter(previous_coro_); }

			co_type get_return_object() noexcept { return co_type::from_promise(*this); }

			Coroutine previous_coro_ = nullptr;
		};
	}
}
#endif // AYR_CORO_PROMISE_HPP_