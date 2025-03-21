#ifndef AYR_NET_EVENTLOOP_HPP
#define AYR_NET_EVENTLOOP_HPP

#include "Channel.hpp"

namespace ayr
{
#if defined(AYR_LINUX)
	UltraEventLoop::UltraEventLoop() : epoll_(), quit(false) {}

	UltraEventLoop::~UltraEventLoop() 
	{
		for (auto&& [fd, ep_ev] : epoll_.epoll_events_.items())
			ayr_desloc(static_cast<Channel*>(ep_ev.data.ptr), 1);
	}

	void UltraEventLoop::add_channel(Channel* channel) 
	{ 
		epoll_.set(channel->fd(), channel, channel->events()); 
	}

	void UltraEventLoop::remove_channel(Channel* channel) 
	{ 
		epoll_.del(channel->fd()); 
		ayr_desloc(channel, 1);
	}

	void UltraEventLoop::stop() { quit = true; }

	// 运行一次事件循环，返回处理的事件数量
	// 返回为0表示超时，返回为-1表示无法运行
	c_size UltraEventLoop::run_once(int timeout_ms)
	{
		if (quit) return -1;
		Array<epoll_event> ep_evs = epoll_.wait(timeout_ms);
		for (auto& ep_ev : ep_evs)
		{
			Channel* channel = static_cast<Channel*>(ep_ev.data.ptr);
			channel->set_revents(ep_ev.events);
			channel->handle();
		}
			
		return ep_evs.size();
	}
#endif // AYR_LINUX
}
#endif // AYR_NET_EVENTLOOP_HPP