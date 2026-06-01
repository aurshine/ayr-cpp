#ifndef AYR_CORO_PROMISE_HPP_
#define AYR_CORO_PROMISE_HPP_

#include <exception>

#include "co_utils.hpp"
#include "../air/Optional.hpp"

namespace ayr
{
	namespace coro
	{
		template<typename T>
		struct PromiseImpl
		{
			using self = PromiseImpl<T>;

			using co_type = std::coroutine_handle<self>;

			std::suspend_always initial_suspend() const noexcept { return {}; }

			std::suspend_always final_suspend() const noexcept { return {}; }

			std::suspend_always yield_value(T value) noexcept { value_ = std::move(value); return {}; }

			void unhandled_exception() { exception_ = std::current_exception(); }

			void return_value(auto&& value) noexcept { value_ = std::forward<decltype(value)>(value); }

			co_type get_return_object() noexcept { return co_type::from_promise(*this); }

			T result()
			{
				if (exception_)
					std::rethrow_exception(exception_);
				if (!value_.has_value())
					RuntimeError("promise witout co_return or co_yield value");
				return std::move(*value_);
			}

			Optional<T> value_;
			std::exception_ptr exception_ = nullptr;
		};

		template<>
		struct PromiseImpl<void>
		{
			using self = PromiseImpl<void>;

			using co_type = std::coroutine_handle<self>;

			std::suspend_always initial_suspend() const noexcept { return {}; }

			std::suspend_always final_suspend() const noexcept { return {}; }

			std::suspend_always yield_value() noexcept { return {}; }

			void unhandled_exception() { exception_ = std::current_exception(); }

			void return_void() noexcept {}

			co_type get_return_object() noexcept { return co_type::from_promise(*this); }

			/*
			* @brief 检查并重新抛出协程内部保存的异常
			*
			* @details 虽然返回值是 void，但仍需 result() 作为异常的检查点。
			* 协程内部通过 co_throw 抛出的异常存储在 exception_ 中，
			* 调用者通过 result() (或 await_resume -> result()) 获取结果时重新抛出。
			* 参考: cppcoro 中 task_promise<void>::result() 采用同样设计。
			*/
			void result()
			{
				if (exception_)
					std::rethrow_exception(exception_);
			}

			std::exception_ptr exception_ = nullptr;
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