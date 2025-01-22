#ifndef AYR_NET_EPOLL_HPP
#define AYR_NET_EPOLL_HPP

#include "Socket.hpp"
#include "../Array.hpp"
#include "../filesystem.hpp"

namespace ayr
{
#if defined(AYR_LINUX)
#include <sys/epoll.h>

	class Epoll : public Object<Epoll>
	{
	public:
		using self = Epoll;

		using super = Object<Epoll>;

		Epoll() : epoll_fd_(epoll_create1(0)) {}

		Epoll(self&& other) : epoll_fd_(std::move(other.epoll_fd_)) {}

		self& operator=(Epoll&& other)
		{
			epoll_fd_ = std::move(other.epoll_fd_);
			return *this;
		}

		// epoll连接数
		c_size size() const { return nfds_; }

		// 添加一个socket到epoll中
		void add(const Socket& socketfd, int events)
		{
			struct epoll_event event;
			event.data.fd = socketfd;
			event.events = events;
			_epoll_ctl(EPOLL_CTL_ADD, socketfd, &event);
			++nfds_;
		}

		// 从epoll中修改一个socket的事件
		void mod(const Socket& socketfd, int events)
		{
			struct epoll_event event;
			event.data.fd = socketfd;
			event.events = events;
			_epoll_ctl(EPOLL_CTL_MOD, socketfd, &event);
		}

		// 从epoll中删除一个socket
		void del(const Socket& socketfd)
		{
			_epoll_ctl(EPOLL_CTL_DEL, socketfd, nullptr);
			--nfds_;
		}

		Array<epoll_event> wait(int timeout)
		{
			Array<epoll_event> events(nfds_);
			int n = epoll_wait(epoll_fd_, events.data(), nfds_, timeout);
			if (n == -1) RuntimeError(get_error_msg());

			Array<epoll_event> result(n);
			for (int i = 0; i < n; ++i)
				result[i] = events[i];
			return result;
		}

	private:
		void _epoll_ctl(int op, const Socket& socketfd, epoll_event* ep_ev)
		{
			if (epoll_ctl(epoll_fd_, op, socketfd, ep_ev) == -1)
				RuntimeError(get_error_msg());
		}

		Socket epoll_fd_;

		c_size nfds_;
	};

#endif // AYR_LINUX
}
#endif