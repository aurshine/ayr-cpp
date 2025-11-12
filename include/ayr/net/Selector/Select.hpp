#ifndef AYR_NET_SELECTOR_SELECT_HPP
#define AYR_NET_SELECTOR_SELECT_HPP

#include <map>
#include <chrono>

#include "IoEvent.hpp"
#include "../../base/Array.hpp"
#include "../../fs/oslib.h"

#if defined(AYR_LINUX)
#include <sys/select.h>
#endif // AYR_LINUX

namespace ayr
{
	namespace net
	{
		class Select : public Object<Select>
		{
			using self = Select;

			using super = Object<Select>;

			// fd_set数组，0位为read_set，1位为write_set，2位为error_set
			// fd_set[3]占128 * 3 = 384字节，使用堆区内存
			std::unique_ptr<fd_set[]> fds_;

			std::map<int, IoEvent> fd_events;
		public:
			Select() :
				fds_(std::make_unique<fd_set[]>(3)),
				fd_events()
			{
				FD_ZERO(read_set());
				FD_ZERO(write_set());
				FD_ZERO(error_set());
			}

			Select(self&& other) noexcept :
				fds_(std::move(other.fds_)),
				fd_events(std::move(other.fd_events)) {
			}

			self& operator=(self&& other) noexcept
			{
				if (this == &other) return *this;
				ayr_destroy(this);
				return *ayr_construct(this, std::move(other));
			}

			// 监听的fd数量
			c_size size() const { return fd_events.size(); }

			// 是否为空
			bool empty() const { return fd_events.empty(); }

			// 是否包含fd
			bool contains(int fd) const { return fd_events.find(fd) != fd_events.end(); }

			/*
			* @brief 添加fd的事件，如果fd已经存在，则更新事件
			*
			* @param fd 要设置的fd
			*
			* @param io_event 要设置的事件
			*/
			void insert(int fd, const IoEvent& io_event)
			{
				FD_CLR(fd, read_set());
				FD_CLR(fd, write_set());
				FD_CLR(fd, error_set());

				if (io_event.registered_events() & IoEvent::READABLE)
					FD_SET(fd, read_set());
				if (io_event.registered_events() & IoEvent::WRITABLE)
					FD_SET(fd, write_set());

				FD_SET(fd, error_set());
				fd_events[fd] = io_event;
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
				FD_CLR(fd, read_set());
				FD_CLR(fd, write_set());
				FD_CLR(fd, error_set());
				fd_events.erase(fd);
				return fd;
			}

			// 移除并关闭fd
			void close(int fd)
			{
				pop(fd);
#if defined(AYR_WIN)
				::closesocket(fd);
#elif defined(AYR_LINUX)
				::close(fd);
#endif
			}

			/*
			* @brief 设置超时，等待事件发送
			*
			* @param timeout_ms 超时时间，单位毫秒
			*
			* @return Array<IoEvent> 发生的事件
			*/
			Array<IoEvent> wait(int timeout_ms)
			{
				auto tmp_fds = std::make_unique<fd_set[]>(3);
				std::memcpy(&tmp_fds[0], read_set(), sizeof(fd_set));
				std::memcpy(&tmp_fds[1], write_set(), sizeof(fd_set));
				std::memcpy(&tmp_fds[2], error_set(), sizeof(fd_set));

				int n = wait_select(&tmp_fds[0], &tmp_fds[1], &tmp_fds[2], timeout_ms);
				Array<IoEvent> results(n);
				for (int i = 0, fd = 0, FD_SIZE = max_fd() + 1; fd < FD_SIZE && i < n; ++fd)
				{
					IoEvent::Flag events = IoEvent::NONE;
					if (FD_ISSET(fd, &tmp_fds[0]))
						events |= IoEvent::READABLE;
					if (FD_ISSET(fd, &tmp_fds[1]))
						events |= IoEvent::WRITABLE;
					if (FD_ISSET(fd, &tmp_fds[2]))
						events |= IoEvent::ERRORABLE;

					if (events != IoEvent::NONE)
					{
						results[i] = fd_events[fd];
						results[i].set_events(events);
						++i;
					}
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

		private:
			int max_fd() const { return fd_events.rbegin()->first; }

			/*
			* @brief 等待事件发生
			*
			* @return int 发生的事件数
			*/
			int wait_select(fd_set* read_set, fd_set* write_set, fd_set* error_set, int timeout_ms)
			{
				timeval timeout{ timeout_ms / 1000, (timeout_ms % 1000) * 1000 };
				timeval* p_tm = ifelse(timeout_ms > 0, &timeout, nullptr);
				int n = select(max_fd() + 1, read_set, write_set, error_set, p_tm);
				if (n == -1)
					RuntimeError(get_error_msg());

				return n;
			}

			fd_set* read_set() { return &fds_[0]; }

			fd_set* write_set() { return &fds_[1]; }

			fd_set* error_set() { return &fds_[2]; }
		};
	}
}

#endif // AYR_NET_SELECTOR_SELECT_HPP