#ifndef AYR_CORO_AWAITER_HPP
#define AYR_CORO_AWAITER_HPP

#include "Promise.hpp"

namespace ayr
{
	namespace coro
	{
		template<typename T>
		class Awaiter : public Object<Awaiter<T>>, public NoCopy
		{
			using self = Awaiter<T>;
		public:
			using promise_type = Promise<T>;

			using co_type = std::coroutine_handle<promise_type>;

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

		template<>
		struct Awaiter<void> : public Object<Awaiter<void>>, public NoCopy
		{
			using self = Awaiter<void>;
		public:
			using promise_type = Promise<void>;

			using co_type = std::coroutine_handle<promise_type>;

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
	}
}
#endif