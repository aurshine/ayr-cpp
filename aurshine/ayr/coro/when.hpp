#ifndef AYR_CORO_WHEN_HPP
#define AYR_CORO_WHEN_HPP

#include <tuple>

#include "CoroLoop.hpp"

namespace ayr
{
	namespace coro
	{
		template<typename T>
		struct WhenOne : public std::suspend_always
		{
			Coroutine await_suspend(Coroutine coroutine) const noexcept
			{
				CoroLoop::add(coro_);
				--count;
				if (count == 0) CoroLoop::add(coroutine);
				return std::noop_coroutine();
			}

			T await_resume() const noexcept { return coro_.promise().result(); }

			size_t& count;

			std::coroutine_handle<Promise<T>> coro_;
		};


		template<typename... Ts>
		std::tuple<Ts...> when_all(Ts&&... coroutines)
		{
			static_assert(sizeof...(Ts) > 0, "No coroutines provided to when");

		}
	}
}
#endif