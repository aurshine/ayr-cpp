#ifndef AYR_CORO_AWAITER_HPP
#define AYR_CORO_AWAITER_HPP

#include "Promise.hpp"

namespace ayr
{
	namespace coro
	{
		// 可等待对象约束
		template<typename A>
		concept Awaitable = requires(A a, std::coroutine_handle<> handle)
		{
			{ a.await_ready() } -> std::convertible_to<bool>;
			{ a.await_suspend(handle) };
			{ a.await_resume() };
			// { A::promise_type };
			// { A::co_type };
		};

		// 可等待对象萃取器
		template<Awaitable A>
		struct AwaitableTraits
		{
			//using promise_type = typename A::promise_type;

			//using co_type = typename A::co_type;

			using T = std::remove_reference_t<decltype(std::declval<A>().await_resume())>;

			using result_type = std::conditional_t<std::is_void_v<T>, _None, T>;
		};
	}
}
#endif