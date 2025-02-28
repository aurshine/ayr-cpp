#ifndef AYR_NET_CHANNEL_HPP
#define AYR_NET_CHANNEL_HPP

#include "Epoll.hpp"

namespace ayr
{
#if defined(AYR_LINUX)
	class Chapoll;
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
			events_(0),
			revents_(0) {}

		~Channel()
		{
			loop_->remove_channel(this);
			socket_.close();
		}

		const Socket& fd() const { return socket_; }

		uint32_t events() const { return events_; }

		uint32_t revents() const { return revents_; }

		void handle() { handle_(this); }

		void enable_read()
		{
			events_ |= EPOLLIN | EPOLLET;
			socket_.setblocking(false);
			loop_->add_channel(this);
		}

		void set_revents(uint32_t revents) { revents_ = revents; }

		void when_handle(const std::function<void(self*)>& handle) { handle_ = handle; }
	};

	// Channel + Epoll的特殊Epoll
	class Chapoll : public Epoll
	{
		using self = Chapoll;

		using super = Epoll;

	public:
		Chapoll() : super() {}

		const Socket& set(const Socket& socketfd, uint32_t events) = delete;

		const Socket& set(const Socket& socketfd, void* ptr, uint32_t events) = delete;

		const Socket& add_channel(Channel* channel)
		{
			return super::set(channel->fd(), channel, channel->events());
		}

		void remove_channel(Channel* channel) { super::del(channel->fd()); }

		Array<Channel*> wait(int timeout)
		{
			int nfds = size();
			Array<epoll_event> events(nfds);
			int n = epoll_wait(epoll_fd(), events.data(), nfds, timeout);
			if (n == -1) RuntimeError(get_error_msg());

			Array<Channel*> channels(n);
			for (int i = 0; i < n; ++i)
			{
				channels[i] = static_cast<Channel*>(events[i].data.ptr);
				channels[i]->set_revents(events[i].events);
			}
			return channels;
		}
	};

#endif // AYR_LINUX
}
#endif // AYR_NET_CHANNEL_HPP