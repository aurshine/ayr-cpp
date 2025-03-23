#ifndef AYR_NET_CHANNEL_HPP
#define AYR_NET_CHANNEL_HPP

#include "Epoll.hpp"

namespace ayr
{
#if defined(AYR_LINUX)
	class UltraEventLoop;

	class Channel : public Object<Channel>
	{
		using self = Channel;

		using super = Object<Channel>;

		Socket socket_;

		uint32_t events_, revents_;

		UltraEventLoop* loop_;

		std::function<void(self*)> handle_;
	public:
		Channel(UltraEventLoop* loop, const Socket& socket) :
			loop_(loop),
			socket_(socket),
			events_(EPOLLIN | EPOLLET | EPOLLHUP | EPOLLRDHUP | EPOLLERR),
			revents_(0)
		{
			socket_.setblocking(false);
		}

		~Channel() { socket_.close(); }

		const Socket& fd() const { return socket_; }

		uint32_t events() const { return events_; }

		uint32_t revents() const { return revents_; }

		void handle() { handle_(this); }

		void set_revents(uint32_t revents) { revents_ = revents; }

		void when_handle(const std::function<void(self*)>& handle) { handle_ = handle; }
		
		UltraEventLoop* loop() const { return loop_; }

		void modeET() { events_ |= EPOLLET; }

		void modeLT() { events_ &= ~EPOLLET; }
	};
#endif // AYR_LINUX
}
#endif // AYR_NET_CHANNEL_HPP