#ifndef AYR_CORO_PROMISE_HPP_
#define AYR_CORO_PROMISE_HPP_

#include <coroutine>

#include <ayr/detail/printer.hpp>
#include <ayr/detail/NoCopy.hpp>

namespace ayr
{
	namespace coro
	{
		using Coroutine = std::coroutine_handle<>;

		struct SuspendPrevious : public std::suspend_always, public Object<SuspendPrevious>
		{
			SuspendPrevious() noexcept = default;

			SuspendPrevious(Coroutine previous_coro) noexcept : previous_coro_(previous_coro) {}

			Coroutine await_suspend(Coroutine coro) const noexcept
			{
				if (previous_coro_)
					return previous_coro_;
				else
					return std::noop_coroutine();
			}

			Coroutine previous_coro_ = nullptr;
		};

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

			SuspendPrevious final_suspend() const noexcept { return SuspendPrevious(previous_coro_); }

			co_type get_return_object() noexcept { return co_type::from_promise(*this); }

			Coroutine previous_coro_ = nullptr;
		};
	}
}
#endif // AYR_CORO_PROMISE_HPP_