#ifndef AYR_NET_EPOLL_HPP
#define AYR_NET_EPOLL_HPP

#include "Socket.hpp"
#include "../Array.hpp"
#include "../filesystem.hpp"
#include "../Dict.hpp"

namespace ayr
{
#if defined(AYR_LINUX)
#include <sys/epoll.h>
	class Epoll : public Object<Epoll>
	{
	public:
		friend class UltraEventLoop;

		using self = Epoll;

		using super = Object<Epoll>;

		Epoll() : epoll_fd_(epoll_create1(0)), size_(0) {}

		~Epoll() { epoll_fd_.close(); }

		// epoll连接数
		c_size size() const { return size_; }

		// 获取epoll的文件描述符
		const Socket& epoll_fd() const { return epoll_fd_; }

		void epoll_ctl(int op, const Socket& socketfd, uint32_t events)
		{
			epoll_event ev;
			ev.data.fd = socketfd.fd();
			ev.events = events;

			unix_epoll_ctl(op, socketfd, &ev);
		}

		void epoll_ctl(int op, const Socket& socketfd, void* ptr, uint32_t events)
		{
			epoll_event ev;
			ev.data.ptr = ptr;
			ev.events = events;

			unix_epoll_ctl(op, socketfd, &ev);
		}

		// 添加一个socket到epoll中
		void add(const Socket& socketfd, uint32_t events)
		{
			epoll_ctl(EPOLL_CTL_ADD, socketfd, events);
		}

		// 添加一个socket到epoll中
		void add(const Socket& socketfd, void* ptr, uint32_t events)
		{
			epoll_ctl(EPOLL_CTL_ADD, socketfd, ptr, events);
		}

		// 从epoll中删除一个socket
		void del(const Socket& socketfd)
		{
			unix_epoll_ctl(EPOLL_CTL_DEL, socketfd, nullptr);
		}

		// 修改一个socket的事件
		void mod(const Socket& socketfd, uint32_t events)
		{
			epoll_ctl(EPOLL_CTL_MOD, socketfd, events);
		}

		// 修改一个socket的事件
		void mod(const Socket& socketfd, void* ptr, uint32_t events)
		{
			epoll_ctl(EPOLL_CTL_MOD, socketfd, ptr, events);
		}

		// 等待事件发生，返回发生的事件数组
		Array<epoll_event> wait(int timeout_ms)
		{
			// 避免epoll 上没有socket
			int nfds = size() + 1;
			Array<epoll_event> events(nfds);
			int n = epoll_wait(epoll_fd_, events.data(), nfds, timeout_ms);
			if (n == -1 && errno != EINTR) RuntimeError(get_error_msg());

			Array<epoll_event> result(n);
			for (int i = 0; i < n; ++i)
				result[i] = events[i];
			return result;
		}
	private:
		void unix_epoll_ctl(int op, const Socket& socketfd, epoll_event* ev)
		{
			if (::epoll_ctl(epoll_fd_.fd(), op, socketfd.fd(), ev) == -1)
				RuntimeError(get_error_msg());

			switch (op)
			{
			case EPOLL_CTL_ADD:
				++size_;
				break;
			case EPOLL_CTL_DEL:
				--size_;
				break;
			}
		}

		Socket epoll_fd_;

		c_size size_;
	};
#endif // AYR_LINUX
}
#endif