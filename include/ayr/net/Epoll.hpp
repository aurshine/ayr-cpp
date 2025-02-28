#ifndef AYR_NET_EPOLL_HPP
#define AYR_NET_EPOLL_HPP

#include "Socket.hpp"
#include "../Array.hpp"
#include "../filesystem.hpp"
#include "../Set.hpp"

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

		~Epoll() { epoll_fd_.close(); }

		// epoll连接数
		c_size size() const { return epoll_sockets_.size(); }

		// 是否包含某个socket
		bool contains(const Socket& socketfd) const { return epoll_sockets_.contains(socketfd); }

		// 获取epoll的文件描述符
		const Socket& epoll_fd() const { return epoll_fd_; }

		// 设置某个socket的事件
		const Socket& set(const Socket& socketfd, uint32_t events)
		{
			struct epoll_event ev;
			ev.data.fd = socketfd.fd();
			ev.events = events;

			return set_impl(socketfd, &ev);
		}

		// 设置某个socket的事件，绑定指针
		const Socket& set(const Socket& socketfd, void* ptr, uint32_t events)
		{
			struct epoll_event ev;
			ev.data.ptr = ptr;
			ev.events = events;

			return set_impl(socketfd, &ev);
		}

		// 从epoll中删除一个socket
		const Socket& del(const Socket& socketfd)
		{
			_epoll_ctl(EPOLL_CTL_DEL, socketfd, nullptr);
			epoll_sockets_.pop(socketfd);
			return socketfd;
		}

		// 等待事件发生，返回发生的事件数组
		Array<epoll_event> wait(int timeout_ms)
		{
			int nfds = size();
			Array<epoll_event> events(nfds);
			int n = epoll_wait(epoll_fd_, events.data(), nfds, timeout_ms);
			if (n == -1) RuntimeError(get_error_msg());

			Array<epoll_event> result(n);
			for (int i = 0; i < n; ++i)
				result[i] = events[i];
			return result;
		}
	private:
		const Socket& set_impl(const Socket& socketfd, epoll_event* ep_ev)
		{
			if (!contains(socketfd))
			{
				epoll_sockets_.insert(socketfd);
				_epoll_ctl(EPOLL_CTL_ADD, socketfd, ep_ev);
			}
			else
			{
				_epoll_ctl(EPOLL_CTL_MOD, socketfd, ep_ev);
			}

			return socketfd;
		}

		void _epoll_ctl(int op, const Socket& socketfd, epoll_event* ep_ev)
		{
			if (epoll_ctl(epoll_fd_.fd(), op, socketfd.fd(), ep_ev) == -1)
				RuntimeError(get_error_msg());
		}

		Set<Socket> epoll_sockets_;

		Socket epoll_fd_;
	};
#endif // AYR_LINUX
}
#endif