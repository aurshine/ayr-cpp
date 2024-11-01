#ifndef AYR_CORO_WHEN_HPP
#define AYR_CORO_WHEN_HPP

#include <tuple>
#include <utility>

#include "CoroLoop.hpp"

namespace ayr
{
	namespace coro
	{
		struct PreviousPromise : public PromiseImpl<Coroutine>
		{
			using self = PreviousPromise;

			using super = PromiseImpl<Coroutine>;

			using co_type = std::coroutine_handle<self>;

			CoroAwaiter final_suspend() const noexcept { return { super::value_ }; }

			co_type get_return_object() noexcept { return co_type::from_promise(*this); }
		};

		template<size_t N>
		struct WhenAllAwaitable : std::suspend_always
		{
			using Task_t = Task<Coroutine, PreviousPromise>;

			WhenAllAwaitable(Task_t* tasks) : tasks_(tasks) {}

			Coroutine await_suspend(Coroutine coroutine) noexcept
			{
				for (c_size i = 0; i < N; ++i)
					CoroLoop::resume(tasks_[i]);
				return std::noop_coroutine();
			}

			Task_t* tasks_;
		};

		template<Awaitable A>
		Task<Coroutine, PreviousPromise> when_all_helper(A&& awaiter, size_t& count, Coroutine previous_coro)
		{
			co_await awaiter;
			if (--count == 0)
				co_return previous_coro;
			co_return std::noop_coroutine();
		}

		template<size_t... Is, Awaitable... As>
		def when_all_impl(std::index_sequence<Is...>, As&&... as) -> Task<void>
		{
			size_t count = sizeof...(As);
			Coroutine cur_coro = co_await CurrentCoro();
			Task<Coroutine, PreviousPromise> tasks[] = { when_all_helper(std::forward<As>(as), count, cur_coro)... };
			co_await WhenAllAwaitable<sizeof...(As)>(tasks);
		}

		template<Awaitable... As>
		def when_all(As&& ... as) -> Task<void>
		{
			co_await when_all_impl(std::make_index_sequence<sizeof...(As)>(), std::forward<As>(as)...);
		}
	}
}
#endif