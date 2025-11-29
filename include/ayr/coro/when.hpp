#ifndef AYR_CORO_WHEN_HPP
#define AYR_CORO_WHEN_HPP

#include <tuple>
#include <array>

#include "CoroLoop.hpp"

namespace ayr
{
	namespace coro
	{
		// 协程结束前恢复返回的协程
		struct ResumeCoroPromise : public PromiseImpl<Coroutine>
		{
			using super = PromiseImpl<Coroutine>;

			using co_type = std::coroutine_handle<ResumeCoroPromise>;

			CoroAwaiter final_suspend() const noexcept { return CoroAwaiter(super::value_); }

			co_type get_return_object() noexcept { return co_type::from_promise(*this); }
		};

		// 接受一个Corotine作为协程返回值，并在该协程结束时唤醒返回的协程
		using ResumeCoroTask = Awaiter<Coroutine, ResumeCoroPromise>;

		// 将awaiter包装成ResumeCoroTask对象
		template<Awaitable A>
		ResumeCoroTask when_all_wrapper(A&& awaiter, int& count, typename AwaitableTraits<A>::result_type& result, Coroutine ret_coro)
		{
			result = co_await awaiter;
			if (--count == 0)
				co_return ret_coro;
			co_return std::noop_coroutine();
		}

		template<size_t N>
		struct WhenAllAwaiter : public std::suspend_always
		{
			using Task_t = ResumeCoroTask;

			WhenAllAwaiter(std::array<Task_t, N>& tasks) : tasks_(tasks) {}

			void await_suspend(Coroutine coroutine) noexcept
			{
				for (Task_t& task : tasks_)
					asyncio.resume(task);
			}

			std::array<Task_t, N>& tasks_;
		};

		template<std::size_t... Is, Awaitable... As>
		def when_all_impl(
			std::index_sequence<Is...>,
			Coroutine ret_coro,
			std::tuple<typename AwaitableTraits<As>::result_type...>& results,
			std::tuple<As...>&& as) -> Task<void>
		{
			int count = sizeof...(As);
			std::array<ResumeCoroTask, sizeof...(As)> tasks = { when_all_wrapper(std::get<Is>(as), count, std::get<Is>(results), ret_coro)... };
			co_await WhenAllAwaiter<tasks.size()>(tasks);
		}

		template<Awaitable... As>
		def when_all(As&& ...as) -> Task<std::tuple<typename AwaitableTraits<As>::result_type...>>
		{
			using Result_t = std::tuple<typename AwaitableTraits<As>::result_type...>;
			Result_t results;
			co_await when_all_impl(
				std::index_sequence_for<As...>(),
				co_await CurrentCoro(),
				results,
				std::make_tuple(std::forward<As>(as)...));

			co_return results;
		}
	}
}
#endif