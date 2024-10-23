#ifndef AYR_CORO_PROMISE_HPP_
#define AYR_CORO_PROMISE_HPP_

#include <coroutine>

#include <ayr/detail/printer.hpp>

namespace ayr
{
	namespace coro
	{
		using Coroutine = std::coroutine_handle<>;

		template<typename T = void>
		class Promise : public Object<Promise<T>>
		{
			using self = Promise<T>;
		public:
			using co_type = std::coroutine_handle<self>;

			std::suspend_always initial_suspend() const noexcept { return {}; }

			std::suspend_always final_suspend() const noexcept { return {}; }

			std::suspend_always yield_value(T value) noexcept { value_ = std::move(value); return {}; }

			void unhandled_exception() { throw; }

			void return_value(T value) noexcept { print("get_return"); value_ = std::move(value); }

			co_type get_return_object() noexcept { return co_type::from_promise(*this); }

			T& result() noexcept { return value_; }

			const T& result() const noexcept { return value_; }
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

			std::suspend_always final_suspend() const noexcept { return {}; }

			std::suspend_always yield_value() noexcept { return {}; }

			void unhandled_exception() { throw; }

			void return_void() noexcept {}

			co_type get_return_object() noexcept { return co_type::from_promise(*this); }
		};
	}
}
#endif // AYR_CORO_PROMISE_HPP_