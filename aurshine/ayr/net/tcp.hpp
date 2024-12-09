#ifndef AYR_NET_TCP_HPP
#define AYR_NET_TCP_HPP

#include "../base/ayr_memory.hpp"
#include "../base/View.hpp"

#include "ServerCallback.hpp"

namespace ayr
{
	class TcpServer : public Object<TcpServer>
	{
	public:
		TcpServer(const char* ip, int port, int family = AF_INET) : socket_(family, SOCK_STREAM)
		{
			socket_.bind(ip, port);
			socket_.listen();
		}

		Socket& accept() { return clients_.append(socket_.accept()); }

		void sendall(const char* data, int size, int flags = 0) const
		{
			for (auto& client : clients())
				client.send(data, size, flags);
		}

		c_size num_clients() const { return clients_.size(); }

		const Socket& client(int index) const { return clients_[index]; }

		auto clients() const -> std::ranges::subrange<DynArray<Socket>::ConstIterator>
		{
			return std::ranges::subrange(clients_.begin(), clients_.end());
		}

		// 断开指定客户端的连接
		void disconnect(const Socket& client)
		{
			c_size index = clients_.index(client);
			if (index == -1)
				RuntimeError("Client not found.");
			clients_.pop(index);
		}
	protected:
		Socket socket_;

		DynArray<Socket> clients_;
	};


	class MiniTcpServer : public TcpServer, public ServerCallback<MiniTcpServer>
	{
		using self = MiniTcpServer;

		using super = TcpServer;

		using super2 = ServerCallback<MiniTcpServer>;
	public:
		MiniTcpServer(const char* ip, int port, int family = AF_INET)
			: super(ip, port, family), read_set(new fd_set), error_set(new fd_set)
		{
			FD_ZERO(read_set);
			FD_ZERO(error_set);
			FD_SET(super::socket_, read_set);
			FD_SET(super::socket_, error_set);
			max_fd = super::socket_;
			super2::set_accept_callback([](self*, const Socket&) {});
			super2::set_recv_callback([](self*, const Socket&, const CString&) {});
			super2::set_error_callback([](self*, const Socket&, const CString&) {});
			super2::set_timeout_callback([](self*) {});
		}

		~MiniTcpServer()
		{
			delete read_set;
			delete error_set;
		}

		Socket& accept()
		{
			Socket& client = super::accept();
			int client_fd = client;
			FD_SET(client_fd, read_set);
			FD_SET(client_fd, error_set);
			max_fd = std::max(max_fd, client_fd);

			return client;
		}

		void push_disconnected(const Socket& client) { disconnected_queue_.append(client); }

		void disconnect(const Socket& client)
		{
			int client_fd = client;
			FD_CLR(client_fd, read_set);
			FD_CLR(client_fd, error_set);
			if (client_fd == max_fd)
			{
				max_fd = 0;
				for (int client : super::clients())
					max_fd = std::max(max_fd, client);
			}
			super::disconnect(client);
		}

		// 阻塞运行服务器，直到服务器停止
		void run(long tv_sec = 2, long tv_usec = 0)
		{
			while (socket_.valid())
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
					super2::timeout_callback_(this);
				else
				{
					if (!check_readset(super::socket_, &read_set_copy) || !check_errorset(super::socket_, &error_set_copy))
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
		// 检查是否有可读事件发生
		// 如果有，则调用相应的回调函数
		// 如果有新连接，则调用accept_callback_
		// 如果有数据到达，则调用recv_callback_
		// 如果回调后调用服务器停止函数，则返回false
		bool check_readset(const Socket& fd, fd_set* read_set)
		{
			if (!FD_ISSET(fd, read_set)) return true;

			if (fd == super::socket_.get_socket())
				super2::accept_callback_(this, accept());
			else
				super2::recv_callback_(this, fd, fd.recv());

			return socket_.valid();
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

			super2::error_callback_(this, fd, errorno2str(errorno));
			return socket_.valid();
		}
	private:
		fd_set* read_set, * error_set;

		// 连接断开的队列
		// 用于在每轮select询问结束后统一断开连接
		DynArray<View> disconnected_queue_;

		// 最大的文件描述符
		int max_fd = 0;
	};

	class UltraTcpServer : public TcpServer, public ServerCallback<UltraTcpServer>
	{
		using self = UltraTcpServer;

		using super = TcpServer;

		using super2 = ServerCallback<UltraTcpServer>;
#if defined(AYR_WIN)
	public:
		UltraTcpServer(const char* ip, int port, int family = AF_INET) : super(ip, port, family) {}
#elif defined(AYR_LINUX)
	public:
		UltraTcpServer(const char* ip, int port, int family = AF_INET) :
			super(ip, port, family), epoll_fd(::epoll_create1(0))
		{
			if (epoll_fd == -1)
				RuntimeError(get_error_msg());

			epoll_ctl(EPOLL_CTL_ADD, super::socket_, EPOLLIN);
		}

		~UltraTcpServer()
		{

		}

		void epoll_ctl(int op, const Socket& fd, int events)
		{
			struct epoll_event event;
			event.events = events;
			event.data.fd = fd;
			if (::epoll_ctl(epoll_fd, op, fd, &event) == -1)
				RuntimeError(get_error_msg());
		}

		Socket& accept()
		{
			Socket& client = super::accept();
			epoll_ctl(EPOLL_CTL_ADD, client, EPOLLIN);
			return client;
		}

		void push_disconnected(const Socket& client) { disconnected_queue_.append(client); }

		void disconnect(const Socket& client)
		{
			epoll_ctl(EPOLL_CTL_DEL, client, nullptr);
			super::disconnect(client);
		}

		void run(long tv_sec = 2, long tv_usec = 0)
		{
			while (true)
			{
				Array<epoll_event> events(super::num_clients() + 1);
				int num_events = epoll_wait(epoll_fd, events.data(), events.size(), tv_sec * 1000 + tv_usec / 1000);
				if (num_events == -1)
			}
			RuntimeError(get_error_msg());

			for (int i = 0; i < num_events; i++)
			{
				if (events[i].data.fd == super::socket_)
					accept_callback_(this, accept());
				else
				{

				}
			}
		}
	}
private:
	Socket epoll_fd;

	DynArray<View> disconnected_queue_;
#endif
	};
}
#endif // AYR_SOCKET_TCP_HPP