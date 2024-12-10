#ifndef AYR_CORO_AWAITER_HPP
#define AYR_CORO_AWAITER_HPP

#include "Promise.hpp"

namespace ayr
{
	namespace coro
	{
		template<typename T, typename P>
		class Awaiter : public Object<Awaiter<T, P>>
		{
			using self = Awaiter<T, P>;
		public:
			using promise_type = P;

			using co_type = promise_type::co_type;

			Awaiter(co_type coro) : coro_(coro) {}

			Awaiter(self&& other) : coro_(other.coro_) { other.coro_ = nullptr; }

			~Awaiter() { if (coro_) coro_.destroy(); }

			Awaiter& operator=(self&& other) noexcept
			{
				if (this == &other) return *this;
				if (coro_) coro_.destroy();
				coro_ = other.coro_;
				other.coro_ = nullptr;
				return *this;
			}

			bool await_ready() const noexcept { return false; }

			co_type await_suspend(Coroutine coroutine) const noexcept { return coro_; }

			T& await_resume() const noexcept { return coro_.promise().result(); }

		protected:
			co_type coro_;
		};

		template<typename P>
		struct Awaiter<void, P> : public Object<Awaiter<void, P>>
		{
			using self = Awaiter<void, P>;
		public:
			using promise_type = P;

			using co_type = promise_type::co_type;

			Awaiter(co_type coro) : coro_(coro) {}

			Awaiter(self&& other) noexcept : coro_(other.coro_) { other.coro_ = nullptr; }

			~Awaiter() { if (coro_) coro_.destroy(); }

			Awaiter& operator=(self&& other) noexcept
			{
				if (this == &other) return *this;
				coro_ = other.coro_;
				other.coro_ = nullptr;
				return *this;
			}

			bool await_ready() const noexcept { return false; }

			co_type await_suspend(Coroutine coroutine) const noexcept { return coro_; }

			void await_resume() const noexcept {}

		protected:
			co_type coro_;
		};

		template<typename A>
		concept Awaitable = requires(A a, std::coroutine_handle<> handle)
		{
			{ a.await_ready() } -> std::convertible_to<bool>;
			{ a.await_suspend(handle) };
			{ a.await_resume() };
		};

		template<Awaitable A>
		struct AwaitableTraits
		{
			using promise_type = A::promise_type;

			using co_type = A::co_type;

			using result_type = std::decay_t<decltype(std::declval<A>().await_resume())>;
		};
	}
}
#endif