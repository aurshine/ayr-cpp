#ifndef AYR_NET_SELECTOR_EPOLL_DATA_HPP
#define AYR_NET_SELECTOR_EPOLL_DATA_HPP

#include "../EventContext.hpp"
#include "../../utils.hpp"

namespace ayr
{
	namespace net
	{
		class EpollData
		{
			using self = EpollData;

			// 监听错误事件, 挂起连接, 关闭连接, 边缘触发
			constexpr static int BASE_EVENT = EPOLLERR | EPOLLHUP | EPOLLRDHUP | EPOLLET | EPOLLONESHOT;

			bool init_;

			int epoll_event_;

			EventContext read_ctx_, write_ctx_;
		public:
			EpollData() : init_(false), epoll_event_(BASE_EVENT), read_ctx_(), write_ctx_() {}

			EpollData(const self& other) : init_(other.init_), epoll_event_(other.epoll_event_), read_ctx_(other.read_ctx_), write_ctx_(other.write_ctx_) {}

			EpollData(self&& other) noexcept : init_(other.init_), epoll_event_(std::exchange(other.epoll_event_, BASE_EVENT)), read_ctx_(std::move(other.read_ctx_)), write_ctx_(std::move(other.write_ctx_)) {}

			self& operator=(const self& other)
			{
				if (this == &other) return *this;
				init_ = other.init_;
				epoll_event_ = other.epoll_event_;
				read_ctx_ = other.read_ctx_;
				write_ctx_ = other.write_ctx_;
				return *this;
			}

			self& operator=(self&& other) noexcept
			{
				if (this == &other) return *this;
				init_ = other.init_;
				epoll_event_ = std::exchange(other.epoll_event_, BASE_EVENT);
				read_ctx_ = std::move(other.read_ctx_);
				write_ctx_ = std::move(other.write_ctx_);
				return *this;
			}

			EventContext& read_ctx() { return read_ctx_; }

			EventContext& write_ctx() { return write_ctx_; }

			bool empty() const { return (epoll_event_ & (EPOLLIN | EPOLLOUT)) == 0; }

			bool has_read() const { return (epoll_event_ & EPOLLIN) != 0; }

			bool has_write() const { return (epoll_event_ & EPOLLOUT) != 0; }

			int event() const { return epoll_event_; }

			int add_read_event(const EventContext& ctx) { return add_event(EPOLLIN, ctx); }

			int add_write_event(const EventContext& ctx) { return add_event(EPOLLOUT, ctx); }

			bool pop_read_event() { return pop_event(EPOLLIN); }

			bool pop_write_event() { return pop_event(EPOLLOUT); }

			// 设置epoll_event结构体的值，并返回该结构体的指针
			epoll_event* set_epoll_event(epoll_event* ev) const
			{
				ev->events = event();
				ev->data.ptr = (void*)this;
				return ev;
			}
		private:
			// 添加事件，返回操作码，EPOLL_CTL_ADD表示添加，EPOLL_CTL_MOD表示修改
			int add_event(int event, const EventContext& ctx)
			{
				if (event == EPOLLIN)
					read_ctx_ = ctx;
				else if (event == EPOLLOUT)
					write_ctx_ = ctx;
				
				epoll_event_ |= event;

				int op = EPOLL_CTL_MOD;
				if (!init_)
				{
					op = EPOLL_CTL_ADD;
					init_ = true;
				}

				return op;
			}

			bool pop_event(int event)
			{
				epoll_event_ &= ~event;
				return empty();
			}
		};
	}
}
#endif // AYR_NET_SELECTOR_EPOLL_DATA_HPP