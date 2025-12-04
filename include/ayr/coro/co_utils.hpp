#ifndef AYR_CORO_CO_UTILS_HPP_
#define AYR_CORO_CO_UTILS_HPP_

#include <coroutine>

#include "../base.hpp"

namespace ayr
{
	namespace coro
	{
		using Coroutine = std::coroutine_handle<>;

		struct CurrentCoro : std::suspend_always
		{
			Coroutine await_suspend(Coroutine coroutine) noexcept
			{
				previous_coro = coroutine;
				return coroutine;
			}

			Coroutine await_resume() const noexcept { return previous_coro; }

		private:
			Coroutine previous_coro = nullptr;
		};

		struct CoroAwaiter : public std::suspend_always
		{
			CoroAwaiter() noexcept {};

			CoroAwaiter(Coroutine coro) noexcept : coro_(coro) {}

			Coroutine await_suspend(Coroutine coro) const noexcept
			{
				if (coro_)
					return coro_;
				else
					return std::noop_coroutine();
			}

			Coroutine coro_ = nullptr;
		};
	}
}
#endif;