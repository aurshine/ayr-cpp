#ifndef AYR_NET_TCP_HPP
#define AYR_NET_TCP_HPP

#include "EventLoopThread.hpp"
#include "../base/View.hpp"

namespace ayr
{
#if defined(AYR_LINUX)
	template<typename DeriverdServer>
	class UltraTcpServer : public Object<UltraTcpServer<DeriverdServer>>
	{
		using self = UltraTcpServer<DeriverdServer>;

		using ReactorLoop = UltraEventLoop;

		ReactorLoop* main_loop;

		Array<UltraEventLoopThread> sub_reactor_threads_;

		async::ThreadPool acceptor_thread_;

		Socket server_socket_;

		int timeout_ms_, next_sub_reactor_id_;

		bool running_ = false;
	public:
		UltraTcpServer(const CString& ip, int port, int num_thread = 1, int timeout_ms = 1000, int family = AF_INET) :
			main_loop(ReactorLoop::loop()),
			sub_reactor_threads_(num_thread),
			acceptor_thread_(1),
			server_socket_(family, SOCK_STREAM),
			timeout_ms_(timeout_ms),
			next_sub_reactor_id_(0)
		{
			server_socket_.bind(ip, port);
			server_socket_.listen();
		}

		~UltraTcpServer()
		{
			stop();
			server_socket_.close();
		}

		// 客户端连接到服务器后调用的回调函数，线程安全
		void on_connected(const Socket& client) {}

		// 客户端断开连接时调用的回调函数
		void on_disconnected(const Socket& client) {}

		// 读事件产生时的回调函数, 用于读取数据
		void on_reading(const Socket& client) {}

		// 写事件产生时的回调函数, 用于发送数据
		void on_writing(const Socket& client) {}

		// 错误事件产生时的回调函数
		void on_error(const Socket& client, const CString& error) { ayr_error(client, error); }

		// 超时的回调函数
		void on_timeout() {}

		// 运行，阻塞函数，只运行一次
		void run()
		{
			running_ = true;

			Channel* acceptor = ayr_make<Channel>(main_loop, server_socket_);
			acceptor->modeLT();
			acceptor->when_handle([&](Channel* channel) { accept(); });
			main_loop->add_channel(acceptor);

			for (auto& sub_thread : sub_reactor_threads_)
				sub_thread = UltraEventLoopThread([&] { run_reactor(); });
			run_reactor();
		}

		// 客户端断开连接，主线程子线程共用函数
		void disconnected(const Socket& client)
		{
			disconnected_by_channel(ReactorLoop::loop()->get_channel(client));
		}

		// 停止服务器，主线程子线程共用函数
		void stop()
		{
			if (!running_) return;

			running_ = false;
			main_loop->stop();
			for (auto& sub_reactor : sub_reactor_threads_)
				sub_reactor.stop();
			acceptor_thread_.stop();
		}
	private:
		// 主线程子线程共用函数
		void run_reactor()
		{
			ReactorLoop* reactor = ReactorLoop::loop();
			while (!reactor->quit())
			{
				c_size num_events = reactor->run_once(timeout_ms_);
				switch (num_events)
				{
				case -1:
					return;
				case 0:
					server().on_timeout();
				}
			}
		}

		// 接受一个socket连接，主线程函数
		void accept()
		{
			Socket client = server_socket_.accept();
			// after_accept进入单独的线程逐个执行
			acceptor_thread_.push(&self::after_accept, this, client);
		}

		// 连接后处理, 主线程函数
		void after_accept(const Socket& client)
		{
			server().on_connected(client);

			ReactorLoop* sub_reactor = main_loop;
			c_size num_sub_reactors = sub_reactor_threads_.size();
			if (num_sub_reactors)
			{
				sub_reactor = sub_reactor_threads_[next_sub_reactor_id_].loop();
				next_sub_reactor_id_ = (next_sub_reactor_id_ + 1) % num_sub_reactors;
			}

			Channel* executor = ayr_make<Channel>(sub_reactor, client);
			executor->when_handle([&](Channel* channel) { execute_event(channel); });
			sub_reactor->add_channel(executor);
		}

		// 客户端断开连接，通过channel，子线程函数
		void disconnected_by_channel(Channel* channel)
		{
			server().on_disconnected(channel->fd());
			ReactorLoop::loop()->remove_channel(channel);
		}

		// 执行事件，子线程函数
		void execute_event(Channel* channel)
		{
			uint32_t events = channel->revents();
			const Socket& client = channel->fd();

			// 错误事件处理
			if (events & (EPOLLERR | EPOLLHUP))
				server().on_error(client, get_error_msg());

			// 断开连接事件处理
			if (events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR))
			{
				disconnected_by_channel(channel);
				return;
			}

			// 读事件处理
			if (events & (EPOLLIN | EPOLLPRI))
				server().on_reading(client);

			// 写事件处理
			if (events & EPOLLOUT)
				server().on_writing(client);
		}

		DeriverdServer& server() { return static_cast<DeriverdServer&>(*this); }
	};
#endif // AYR_LINUX
}
#endif // AYR_SOCKET_TCP_HPP