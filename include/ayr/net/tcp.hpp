#ifndef AYR_NET_TCP_HPP
#define AYR_NET_TCP_HPP

#include "../base/ayr_memory.hpp"
#include "../base/View.hpp"

#include "Channel.hpp"
#include "EventLoop.hpp"
#include "ServerCallback.hpp"

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
	class UltraTcpServer
	{
		using self = UltraTcpServer<DeriverdServer>;

		/ loop_;

		Socket server_socket_;
	public:
		UltraTcpServer(int port, int family = AF_INET) :
			loop_(),
			server_socket_(family, SOCK_STREAM)
		{
			server_socket_.bind("127.0.0.1", port);
			server_socket_.listen();
		}

		~UltraTcpServer() { server_socket_.close(); }

		// 客户端连接到服务器后调用的回调函数
		void on_connected(const Socket& client) {}

		// 客户端断开连接时调用的回调函数
		void on_disconnected(const Socket& client) {}

		// 读事件产生时的回调函数, 返回读取的数据
		CString on_reading(const Socket& client) {}

		// 写事件产生时的回调函数, 用于发送数据
		void on_writing(const Socket& client, const CString& message) {}

		// 得到读数据的回调函数, 用于处理读到的数据
		void on_message(const Socket& client, const CString& message) {}

		// 运行
		void run()
		{
			Channel* channel = ayr_make<Channel>(&loop_, server_socket_);
			channel->enable_read();
			channel->when_handle([&](Channel* channel){ server().on_accept(); });

			loop_.run();
		}

		// 接受一个socket连接
		void on_accept()
		{
			Socket client_socket = server_socket_.accept();
			on_connected(client_socket);

			Channel* channel = ayr_make<Channel>(&loop, client_socket);
			channel->enable_read();
			channel->when_handle([&](Channel* channel){ server().execute_read(channel); });
		}

		// 执行读事件
		void execute_read(Channel* channel)
		{
			Socket& client = channel->fd();
			CString message = server().on_reading(client);
			if (message.empty())
			{
				server().on_disconnected(client);
				delete channel;
			}
			else
				server().on_message(client, message);
		}

		DeriverdServer& server() { return static_cast<DeriverdServer&>(*this); }
	};
#endif // AYR_LINUX
}
#endif // AYR_SOCKET_TCP_HPP