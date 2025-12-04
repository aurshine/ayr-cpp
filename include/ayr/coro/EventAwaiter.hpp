#ifndef AYR_CORO_EVENTAWAITER_HPP
#define AYR_CORO_EVENTAWAITER_HPP

#include "Task.hpp"
#include "../net/Selector/IoWaiter.hpp"

namespace ayr
{
	namespace coro
	{
		// 事件驱动可等待对象
		class EventAwaiter
		{
			using self = EventAwaiter;

			int fd_;

			net::IoEvent::Flag flags_;

			net::IoWaiter* io_waiter_;

			// 记录最后一个被挂起的协程
			Coroutine last_coro_ = nullptr;
		public:
			EventAwaiter() : fd_(-1), flags_(net::IoEvent::NONE), io_waiter_(nullptr) {}

			EventAwaiter(int fd, net::IoEvent::Flag flags, net::IoWaiter* io_waiter) :
				fd_(fd),
				flags_(flags),
				io_waiter_(io_waiter) {
			}

			EventAwaiter(const self& other) : fd_(other.fd_), flags_(other.flags_), io_waiter_(other.io_waiter_) {}

			~EventAwaiter() { io_waiter_->pop(fd_); }

			self& operator=(const self& other)
			{
				if (this == &other) return *this;
				return *ayr_construct(this, other);
			}

			bool await_ready() const noexcept { return fd_ == -1 || io_waiter_ == nullptr; }

			void await_suspend(Coroutine coro) noexcept
			{
				if (last_coro_ == coro) return;
				last_coro_ = coro;
				// 只在协程发生变化时才重新注册事件
				io_waiter_->insert(fd_, net::IoEvent(flags_, coro));
			}

			void await_resume() const noexcept {}
		};
	}
}
#endif // AYR_CORO_EVENTAWAITER_HPP