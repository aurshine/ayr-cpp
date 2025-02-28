#ifndef AYR_NET_EVENTLOOP_HPP
#define AYR_NET_EVENTLOOP_HPP

#include "Channel.hpp"

namespace ayr
{
#if defined(AYR_LINUX)
	class UltraEventLoop : public Object<UltraEventLoop>
	{
		using self = UltraEventLoop;

		using super = Object<UltraEventLoop>;

		Chapoll chapoll_;

		bool quit;
	public:
		UltraEventLoop() : chapoll_(), quit(false) {}

		// 添加一个Channel到EventLoop中
		void add_channel(Channel* channel) { chapoll_.add_channel(channel); }

		// 从EventLoop中移除一个Channel
		void remove_channel(Channel* channel) { chapoll_.remove_channel(channel); }

		// 停止EventLoop
		void quit_loop() { quit = true; }

		// 运行EventLoop
		void run(int timeout)
		{
			while (!quit)
				for (auto& channel : chapoll_.wait(timeout))
					channel->handle();
		}
	};
#endif // AYR_LINUX
}
#endif // AYR_NET_EVENTLOOP_HPP