#ifndef AYR_NET_TCP_HPP
#define AYR_NET_TCP_HPP

#include <functional>

#include "../base/ayr_memory.hpp"

#include "Socket.hpp"

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

		void send(int index, const char* data, int size, int flags = 0) const { client(index).send(data, size, flags); }

		void sendall(const char* data, int size, int flags = 0) const
		{
			for (auto& client : clients())
				client.send(data, size, flags);
		}

		CString recv(int index, int flags = 0) const { return client(index).recv(flags); }

		c_size num_clients() const { return clients_.size(); }

		const Socket& client(int index) const { return clients_[index]; }

		auto clients() const -> std::ranges::subrange<DynArray<Socket>::ConstIterator>
		{
			return std::ranges::subrange(clients_.begin(), clients_.end());
		}

		void disconnect(const Socket& client) { clients_.pop(clients_.index(client)); }

		// 关闭服务器
		void close() { socket_.close(); clients_.clear(); }
	protected:
		Socket socket_;

		DynArray<Socket> clients_;
	};


	class MiniTcpServer : public TcpServer
	{
		using self = MiniTcpServer;

		using super = TcpServer;
	public:
		MiniTcpServer(const char* ip, int port, int family = AF_INET)
			: super(ip, port, family), read_set(new fd_set), error_set(new fd_set)
		{
			FD_ZERO(read_set);
			FD_ZERO(error_set);
			FD_SET(super::socket_, read_set);
			FD_SET(super::socket_, error_set);
			max_fd = super::socket_;
			set_accept_callback([](self*, const Socket&) {});
			set_recv_callback([](self*, const Socket&, const CString&) {});
			set_error_callback([](self*, const Socket&, const CString&) {});
			set_timeout_callback([](self*) {});
		}

		~MiniTcpServer()
		{
			delete read_set;
			delete error_set;
		}

		void set_accept_callback(const std::function<void(self*, const Socket&)>& accept_callback)
		{
			accept_callback_ = accept_callback;
		}

		void set_recv_callback(const std::function<void(self*, const Socket&, const CString&)>& recv_callback)
		{
			recv_callback_ = recv_callback;
		}

		void set_error_callback(const std::function<void(self*, const Socket&, const CString&)>& error_callback)
		{
			error_callback_ = error_callback;
		}

		void set_timeout_callback(const std::function<void(self*)>& timeout_callback)
		{
			timeout_callback_ = timeout_callback;
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

		void disconnect(const Socket& client)
		{
			super::disconnect(client);
			int client_fd = client;
			FD_CLR(client_fd, read_set);
			FD_CLR(client_fd, error_set);
			if (client_fd == max_fd)
			{
				max_fd = 0;
				for (int client : super::clients())
					max_fd = std::max(max_fd, client);
			}
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
					timeout_callback_(this);
				else
				{
					if (!check_readset(super::socket_, &read_set_copy) || !check_errorset(super::socket_, &error_set_copy))
						return;

					for (auto& client : super::clients())
						if (!check_readset(client, &read_set_copy) || !check_errorset(client, &error_set_copy))
							return;
				}
			}
		}

		void close()
		{
			max_fd = 0;
			FD_ZERO(read_set);
			FD_ZERO(error_set);
			super::close();
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
				accept_callback_(this, accept());
			else
				recv_callback_(this, fd, fd.recv());

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

			error_callback_(this, fd, errorno2str(errorno));
			return socket_.valid();
		}
	private:
		fd_set* read_set, * error_set;

		// 最大的文件描述符
		int max_fd = 0;

		// 客户端接受后的回调函数
		// 第一个参数为当前对象，第二个参数为新连接的Socket对象
		std::function<void(self*, const Socket&)> accept_callback_;

		// 接收到数据后的回调函数
		// 第一个参数为当前对象，第二个参数为发送数据的Socket对象，第三个参数为接收到的数据
		std::function<void(self*, const Socket&, const CString&)> recv_callback_;

		// 发生错误后的回调函数
		// 第一个参数为当前对象，第二个参数为发生错误的Socket对象, 第三参数为错误信息
		std::function<void(self*, const Socket&, const CString&)> error_callback_;

		// 超时后的回调函数
		// 第一个参数为当前对象
		std::function<void(self*)> timeout_callback_;
	};
}
#endif // AYR_SOCKET_TCP_HPP