#ifndef AYR_NET_SELECTOR_EPOLL_HPP
#define AYR_NET_SELECTOR_EPOLL_HPP

#include <sys/epoll.h>

#include "IoEvent.hpp"
#include "../../fs/oslib.h"
#include "../../Dict.hpp"

namespace ayr
{
	class Epoll : public Object<Epoll>
	{
		using self = Epoll;

		using super = Object<self>;

		int epoll_fd_;

		// 保存fd和事件的映射
		Dict<int, IoEvent> fd_events;
	public:
		Epoll() : epoll_fd_(::epoll_create1(0)) {}

		Epoll(self&& other) :
			epoll_fd_(std::exchange(other.epoll_fd_, -1)),
			fd_events(std::move(other.fd_events)) {
		}

		~Epoll() { ::close(epoll_fd_); }

		self& operator=(self&& other)
		{
			if (this == &other) return *this;

			ayr_destroy(this);
			return *ayr_construct(this, std::move(other));
		}

		// 监听的fd数量
		c_size size() const { return fd_events.size(); }

		/*
		* @brief 注册一个socket到epoll中
		*
		* @param fd 要注册的socket的文件描述符
		*
		* @param events 要注册的事件
		*
		* @note fd 必须是没添加到epoll中的
		*/
		void add(int fd, const IoEvent& io_event)
		{
			struct epoll_event ev;
			ev.data.ptr = (void*)&fd_events.insert(fd, io_event);
			// 默认监听错误事件，挂起连接
			ev.events = EPOLLERR | EPOLLHUP | EPOLLRDHUP;
			// 读事件，ET模式
			if (io_event.registered_events() & IoEvent::READABLE)
				ev.events |= EPOLLIN | EPOLLET;
			// 写事件
			if (io_event.registered_events() & IoEvent::WRITABLE)
				ev.events |= EPOLLOUT;

			::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
		}

		/*
		* @brief 从epoll中删除一个socket，但不关闭socket
		*
		* @param fd 要删除的socket的文件描述符
		*
		* @return 被删除的socket
		*/
		int pop(int fd)
		{
			::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
			fd_events.pop(fd);
			return fd;
		}

		// 移除并关闭fd
		void close(int fd)
		{
			pop(fd);
			::close(fd);
		}

		Array<IoEvent> wait(int timeout_ms)
		{
			Array<epoll_event> evs(size());
			int n = ::epoll_wait(epoll_fd_, evs.data(), evs.size(), timeout_ms);
			if (n == -1 && errno != EINTR)
				RuntimeError(get_error_msg());

			Array<IoEvent> results(n);
			for (int i = 0; i < n; ++i)
			{
				results[i] = *static_cast<IoEvent*>(evs[i].data.ptr);

				IoEvent::Flag events = IoEvent::NONE;
				// 记录读事件
				if (evs[i].events & EPOLLIN)
					events |= IoEvent::READABLE;
				// 记录写事件
				if (evs[i].events & EPOLLOUT)
					events |= IoEvent::WRITABLE;
				// 记录错误事件
				if (evs[i].events & EPOLLERR)
					events |= IoEvent::ERRORABLE;
				results[i].set_events(events);
			}

			return results;
		}
	};
}

#endif // AYR_NET_SELECTOR_EPOLL_HPP