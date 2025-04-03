#ifndef AYR_NET_EVENTLOOP_HPP
#define AYR_NET_EVENTLOOP_HPP

#include "Channel.hpp"
#include "../async/ThreadPool.hpp"

namespace ayr
{
#if defined(AYR_LINUX)
	class UltraEventLoop : public Object<UltraEventLoop>
	{
		using self = UltraEventLoop;

		using super = Object<UltraEventLoop>;

		Epoll epoll_;

		bool quit_;

		Dict<Socket, Channel*> channel_map;

		std::mutex mtx;

		std::thread::id thread_id_;

		UltraEventLoop() : epoll_(), quit_(false), channel_map(), thread_id_(std::this_thread::get_id()) {}
	public:
		bool quit() 
		{ 
			std::lock_guard<std::mutex> lock(mtx);
			return quit_; 
		}

		void stop() 
		{
			std::lock_guard<std::mutex> lock(mtx);
			quit_ = true; 
		}

		static self* loop()
		{
			thread_local static self instance;
			return &instance;
		}

		~UltraEventLoop()
		{
			// 释放所有Channel
			for (auto& channel : channel_map.values())
				ayr_desloc(channel);
		}

		// 从当前事件循环获取socket对应的Channel
		Channel* get_channel(const Socket& socket)
		{
			check_inthread();
			std::lock_guard<std::mutex> lock(mtx);
			return channel_map.get(socket.fd());
		}

		// 添加一个Channel到事件循环中, 该函数由主线程调用
		void add_channel(Channel* channel)
		{
			check_inloop(channel);
			std::lock_guard<std::mutex> lock(mtx);
			epoll_.add(channel->fd(), channel, channel->events());
			channel_map.insert(channel->fd(), channel);
		}

		// 从事件循环中移除一个Channel
		void remove_channel(Channel* channel)
		{
			check_inthread();
			check_inloop(channel);
			std::lock_guard<std::mutex> lock(mtx);
			epoll_.del(channel->fd());
			channel_map.pop(channel->fd());
			ayr_desloc(channel);
		}

		// 运行一次事件循环，返回处理的事件数量
		// 返回为0表示超时，返回为-1表示无法运行
		c_size run_once(int timeout_ms)
		{
			check_inthread();
			if (quit()) return -1;
			Array<epoll_event> ep_evs = epoll_.wait(timeout_ms);
			for (auto& ep_ev : ep_evs)
			{
				Channel* channel = static_cast<Channel*>(ep_ev.data.ptr);
				channel->set_revents(ep_ev.events);
				channel->handle();
			}

			return ep_evs.size();
		}
	private:
		void check_inloop(Channel* channel) const
		{
			if (channel->loop() != this)
				RuntimeError("Channel is not created by this loop");
		}

		void check_inthread() const
		{
			if (std::this_thread::get_id() != thread_id_)
				RuntimeError("Function must be called in the loop thread");
		}
	};
#endif // AYR_LINUX
}
#endif // AYR_NET_EVENTLOOP_HPP