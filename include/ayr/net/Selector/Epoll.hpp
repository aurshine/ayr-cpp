#ifndef AYR_NET_SELECTOR_EPOLL_HPP
#define AYR_NET_SELECTOR_EPOLL_HPP

#include <sys/epoll.h>

#include <chrono>

#include "IoEvent.hpp"
#include "../../fs/oslib.h"
#include "../../Dict.hpp"

namespace ayr
{
	namespace net
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

			// 是否为空
			bool empty() const { return size() == 0; }

			// 是否包含fd
			bool contains(int fd) const { return fd_events.contains(fd); }

			/*
			* @brief 添加一个fd事件到epoll中，如果fd已经存在，则更新事件
			*
			* @param fd 要注册的socket的文件描述符
			*
			* @param events 要注册的事件
			*
			*/
			void insert(int fd, const IoEvent& io_event)
			{
				struct epoll_event ev;
				ev.data.ptr = (void*)&fd_events.insert(fd, io_event);
				// 默认监听错误事件，挂起连接
				ev.events = EPOLLERR | EPOLLHUP | EPOLLRDHUP;
				// 读事件，LT模式
				if (io_event.registered_events() & IoEvent::READABLE)
					ev.events |= EPOLLIN;
				// 写事件
				if (io_event.registered_events() & IoEvent::WRITABLE)
					ev.events |= EPOLLOUT;

				if (contains(fd))
					::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
				else
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

			/*
			* @brief 等待epoll事件
			*
			* @param timeout_ms 超时时间，单位毫秒
			*
			* @return 发生的事件列表
			*/
			Array<IoEvent> wait(int timeout_ms)
			{
				Array<epoll_event> evs(size());
				int n = ::epoll_wait(epoll_fd_, evs.data(), evs.size(), timeout_ms);
				if (n == -1)
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

			/*
			* @brief 等待epoll事件直到到达指定时间
			*
			* @details 超时时间为距离time_point的毫秒数，如果已经超时则立刻返回
			*
			* @param time_point 超时时间点
			*
			* @return 发生的事件列表
			*/
			Array<IoEvent> wait_until(std::chrono::steady_clock::time_point time_point)
			{
				int timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_point - std::chrono::steady_clock::now()).count();
				return wait(std::max(timeout_ms, 0));
			}
		};
	}
}

#endif // AYR_NET_SELECTOR_EPOLL_HPP