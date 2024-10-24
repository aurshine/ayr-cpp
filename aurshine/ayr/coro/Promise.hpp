#ifndef AYR_CORO_PROMISE_HPP_
#define AYR_CORO_PROMISE_HPP_

#include <coroutine>

#include <ayr/detail/printer.hpp>

namespace ayr
{
	namespace coro
	{
		using Coroutine = std::coroutine_handle<>;

		struct SuspendPrevious : public std::suspend_always
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


		template<typename T = void>
		class Promise : public Object<Promise<T>>
		{
			using self = Promise<T>;
		public:
			using co_type = std::coroutine_handle<self>;

			std::suspend_always initial_suspend() const noexcept { return {}; }

			SuspendPrevious final_suspend() const noexcept { return SuspendPrevious(previous_coro_); }

			std::suspend_always yield_value(T value) noexcept { value_ = std::move(value); return {}; }

			void unhandled_exception() { throw; }

			void return_value(T value) noexcept { value_ = std::move(value); }

			co_type get_return_object() noexcept { return co_type::from_promise(*this); }

			T& result() noexcept { return value_; }

			const T& result() const noexcept { return value_; }

			Coroutine previous_coro_ = nullptr;
		private:
			T value_;
		};


		template <>
		class Promise<void> : public Object<Promise<void>>
		{
			using self = Promise<void>;
		public:
			using co_type = std::coroutine_handle<self>;

			std::suspend_always initial_suspend() const noexcept { return {}; }

			SuspendPrevious final_suspend() const noexcept { return SuspendPrevious(previous_coro_); }

			std::suspend_always yield_value() noexcept { return {}; }

			void unhandled_exception() { throw; }

			void return_void() noexcept {}

			co_type get_return_object() noexcept { return co_type::from_promise(*this); }

			Coroutine previous_coro_ = nullptr;
		};
	}
}
#endif // AYR_CORO_PROMISE_HPP_