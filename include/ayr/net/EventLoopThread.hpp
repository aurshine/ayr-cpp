#ifndef AYR_NET_EVENTLOOPTHREAD_HPP
#define AYR_NET_EVENTLOOPTHREAD_HPP

#include "EventLoop.hpp"
#include "../async/ThreadPool.hpp"

namespace ayr
{
#ifdef AYR_LINUX
	class UltraEventLoopThread : public Object<UltraEventLoopThread>
	{
		using self = UltraEventLoopThread;

		using super = Object<self>;

		using Loop = UltraEventLoop;

		using Task = std::function<void()>;

		std::thread thread_;

		Loop* loop_ = nullptr;
	public:
		UltraEventLoopThread() : thread_() {}

		UltraEventLoopThread(const std::function<void()>& fn)
		{
			std::promise<Loop*> promise;
			std::future<Loop*> future = promise.get_future();
			thread_ = std::thread([&]() {
				promise.set_value(Loop::loop());
				fn();
				});
			loop_ = future.get();
		}

		UltraEventLoopThread(self&& other) noexcept :
			thread_(std::move(other.thread_)),
			loop_(other.loop_)
		{
			other.loop_ = nullptr;
		}

		~UltraEventLoopThread() { stop(); }

		self& operator=(self&& other)
		{
			thread_ = std::move(other.thread_);
			loop_ = other.loop_;
			other.loop_ = nullptr;
			return *this;
		}

		void stop()
		{
			if (loop_) loop_->stop();
			if (thread_.joinable()) thread_.join();
		}

		Loop* loop() const { return loop_; }
	};
#endif // AYR_LINUX
}
#endif // AYR_NET_EVENTLOOPTHREAD_HPP