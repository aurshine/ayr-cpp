#ifndef AYR_NET_TCP_HPP
#define AYR_NET_TCP_HPP

#include "EventLoop.hpp"
#include "ServerCallback.hpp"
#include "Select.hpp"
#include "../async/ThreadPool.hpp"
#include "../base/ayr_memory.hpp"
#include "../base/View.hpp"

namespace ayr
{
	class TcpServer : public Object<TcpServer>
	{
		using self = TcpServer;
	public:
		TcpServer(const char* ip, int port, int family = AF_INET) : server_socket_(family, SOCK_STREAM)
		{
			server_socket_.bind(ip, port);
			server_socket_.listen();
		}

		// 关闭服务器并断开所有客户端的连接
		void close()
		{
			server_socket_.close();
			clients_.each([](const Socket& client) { client.close(); });
			clients_.clear();
		}

		Socket& accept() { return clients_.append(server_socket_.accept()); }

		c_size num_clients() const { return clients_.size(); }

		const Socket& client(int index) const { return clients_[index]; }

		const DynArray<Socket>& clients() const { return clients_; }

		// 断开指定客户端的连接
		void disconnect(const Socket& client)
		{
			c_size index = clients_.index(client);
			if (index == -1)
				RuntimeError("Client not found.");
			client.close();
			clients_.pop(index);
		}
	protected:
		Socket server_socket_;

		DynArray<Socket> clients_;
	};

	// select模型的TCP服务器
	class MiniTcpServer : public TcpServer, public ServerCallback<MiniTcpServer>
	{
		using self = MiniTcpServer;

		using super = TcpServer;

		using CallBack = ServerCallback<MiniTcpServer>;
	public:
		MiniTcpServer(const char* ip, int port, int family = AF_INET)
			: super(ip, port, family),
			CallBack(),
			read_set(ayr_make<fd_set>()),
			error_set(ayr_make<fd_set>())
		{
			FD_ZERO(read_set);
			FD_ZERO(error_set);
			FD_SET(super::server_socket_, read_set);
			FD_SET(super::server_socket_, error_set);
			max_fd = super::server_socket_;
		}

		~MiniTcpServer() { close(); }

		Socket& accept()
		{
			Socket& client = super::accept();
			int client_fd = client.fd();
			FD_SET(client_fd, read_set);
			FD_SET(client_fd, error_set);
			max_fd = std::max(max_fd, client_fd);

			return client;
		}

		// 等待删除的客户端队列
		void push_disconnected(const Socket& client) { disconnected_queue_.append(client); }

		// 阻塞运行服务器，直到服务器停止
		void run(long tv_sec = 2, long tv_usec = 0)
		{
			while (valid_)
			{
				fd_set read_set_copy = *read_set;
				fd_set error_set_copy = *error_set;
				timeval timeout = { tv_sec, tv_usec };

				int num_ready = select(max_fd + 1,
					&read_set_copy, nullptr, &error_set_copy,
					ifelse(tv_sec < 0 || tv_usec < 0, nullptr, &timeout));

				if (num_ready == -1)
					RuntimeError(get_error_msg());
				else if (num_ready == 0)
					CallBack::timeout_callback_(this);
				else
				{
					if (!check_readset(super::server_socket_, &read_set_copy) || !check_errorset(super::server_socket_, &error_set_copy))
						return;

					for (auto& client : super::clients())
						if (!check_readset(client, &read_set_copy) || !check_errorset(client, &error_set_copy))
							return;
				}

				if (disconnected_queue_.size())
				{
					for (auto& client : disconnected_queue_)
						disconnect(client);
					disconnected_queue_.clear();
				}
			}
		}
	private:
		void close()
		{
			ayr_desloc(read_set, 1);
			ayr_desloc(error_set, 1);
			super::close();
			valid_ = false;
		}

		void disconnect(const Socket& client)
		{
			CallBack::disconnect_callback_(this, client);

			int client_fd = client.fd();
			FD_CLR(client_fd, read_set);
			FD_CLR(client_fd, error_set);
			super::disconnect(client);

			if (client_fd == max_fd)
			{
				max_fd = 0;
				for (int client : super::clients())
					max_fd = std::max(max_fd, client);
			}
		}

		// 检查是否有可读事件发生
		// 如果有，则调用相应的回调函数
		// 如果有新连接，则调用accept_callback_
		// 如果有数据到达，则调用recv_callback_
		// 如果回调后调用服务器停止函数，则返回false
		bool check_readset(const Socket& socket, fd_set* read_set)
		{
			if (!FD_ISSET(socket, read_set)) return true;
			if (socket == super::server_socket_)
				CallBack::accept_callback_(this, accept());
			else
				CallBack::recv_callback_(this, socket);

			return valid_;
		}

		// 检查是否有异常事件发生
		// 如果有则调用error_callback_
		// 如果回调后调用服务器停止函数，则返回false
		bool check_errorset(const Socket& fd, fd_set* error_set)
		{
			if (!FD_ISSET(fd, error_set)) return true;

			int errorno = 0;
			socklen_t len = sizeof(errorno);
			if (fd.getsockopt(SOL_SOCKET, SO_ERROR, &errorno, &len) == -1)
				RuntimeError(get_error_msg());

			CallBack::error_callback_(this, fd, errorno2str(errorno));
			return valid_;
		}
	private:
		fd_set* read_set, * error_set;

		// 连接断开的队列
		// 用于在每轮select询问结束后统一断开连接
		DynArray<Socket> disconnected_queue_;

		// 最大的文件描述符
		int max_fd = 0;

		// 服务器是否有效
		bool valid_ = true;
	};

#if defined(AYR_LINUX)
	template<typename DeriverdServer>
	class UltraTcpServer : public Object<UltraTcpServer<DeriverdServer>>
	{
		using self = UltraTcpServer<DeriverdServer>;

		using ReactorLoop = UltraEventLoop;

		ReactorLoop main_reactor_;

		Array<ReactorLoop> sub_reactors_;

		thread::ThreadPool reactor_thread_pool_, acceptor_thread_pool_;

		Socket server_socket_;

		int timeout_ms_, next_sub_reactor_id_;
	public:
		UltraTcpServer(int port, int num_thread = 1, int timeout_ms = -1, int family = AF_INET) :
			main_reactor_(),
			sub_reactors_(num_thread),
			reactor_thread_pool_(num_thread),
			acceptor_thread_pool_(1),
			timeout_ms_(timeout_ms),
			server_socket_(family, SOCK_STREAM),
			next_sub_reactor_id_(0)
		{
			server_socket_.bind("127.0.0.1", port);
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

		// 运行
		void run()
		{
			auto acceptor = main_reactor_.add_channel(server_socket_, [&](Channel* channel) { accept(); });
			acceptor->modeLT();

			for (auto& sub_reactor : sub_reactors_)
				reactor_thread_pool_.push([&] { run_reactor(sub_reactor); });
			run_reactor(main_reactor_);
		}

		// 停止服务器
		void stop()
		{
			main_reactor_.stop();
			for (auto& sub_reactor : sub_reactors_)
				sub_reactor.stop();
			reactor_thread_pool_.stop();
			acceptor_thread_pool_.stop();
		}
	private:
		void run_reactor(ReactorLoop& reactor)
		{
			while (true)
			{
				c_size num_events = reactor.run_once(timeout_ms_);
				switch (num_events)
				{
				case -1:
					return;
				case 0:
					server().on_timeout();
				}
			}
		}

		// 接受一个socket连接
		void accept()
		{
			Socket client = server_socket_.accept();
			// after_accept进入单独的线程逐个执行
			acceptor_thread_pool_.push(&self::after_accept, this, client);
		}

		// 连接后处理, 线程安全
		void after_accept(const Socket& client)
		{
			server().on_connected(client);

			ReactorLoop* sub_reactor = &main_reactor_;
			c_size num_sub_reactors = sub_reactors_.size();
			if (num_sub_reactors)
			{
				sub_reactor = &sub_reactors_[next_sub_reactor_id_];
				next_sub_reactor_id_ = (next_sub_reactor_id_ + 1) % num_sub_reactors;
			}
				
			auto executor = sub_reactor->add_channel(client, [&](Channel* channel) { execute_event(channel); });
		}

		// 客户端断开连接
		void disconnected(Channel* channel)
		{
			server().on_disconnected(channel->fd());
			channel->loop()->remove_channel(channel);
		}

		// 执行事件
		void execute_event(Channel* channel)
		{
			uint32_t events = channel->revents();
			const Socket& client = channel->fd();

			// 错误事件处理
			if (events & EPOLLERR)
				server().on_error(client, get_error_msg());

			// 断开连接事件处理
			if (events & EPOLLEND)
			{
				disconnected(channel);
				return;
			}

			// 读事件处理
			if (events & EPOLLIN)
				server().on_reading(client);

			// 写事件处理
			if (events & EPOLLOUT)
				server().on_writing(client);
		}

		DeriverdServer& server() { return static_cast<DeriverdServer&>(*this); }

		constexpr static uint32_t EPOLLEND = EPOLLHUP | EPOLLRDHUP | EPOLLERR;
	};
#endif // AYR_LINUX
}
#endif // AYR_SOCKET_TCP_HPP