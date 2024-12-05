#ifndef AYR_NET_TCP_HPP
#define AYR_NET_TCP_HPP

#include <vector>

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

		Socket& accept()
		{
			clients_.push_back(socket_.accept());
			return clients_.back();
		}

		void send(int index, const char* data, int size, int flags = 0) const { client(index).send(data, size, flags); }

		void sendall(const char* data, int size, int flags = 0) const
		{
			for (const auto& client : clients_)
				client.send(data, size, flags);
		}

		CString recv(int index, int flags = 0) const { return client(index).recv(flags); }

		const Socket& client(int index) const { return clients_[index]; }

		auto clients() const { return std::ranges::subrange(clients_.begin(), clients_.end()); }

	protected:
		Socket socket_;

		std::vector<Socket> clients_;
	};

	class MiniTcpServer : public TcpServer
	{
		using self = MiniTcpServer;

		using super = TcpServer;
	public:
		MiniTcpServer(const char* ip, int port, int family = AF_INET)
			: super(ip, port, family), read_set(new fd_set), write_set(nullptr), error_set(new fd_set)
		{
			FD_ZERO(read_set);
			FD_ZERO(write_set);
			FD_ZERO(error_set);
			FD_SET(super::socket_, read_set);
			FD_SET(super::socket_, error_set);
			max_fd = super::socket_;
		}

		~MiniTcpServer()
		{
			delete read_set;
			delete write_set;
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

		// 停止服务器
		void stop() { socket_.close(); }

		// 运行服务器，直到服务器停止
		void run(long tv_sec, long tv_usec = 0)
		{
			while (socket_.valid())
			{
				fd_set read_set_copy = *read_set;
				fd_set error_set_copy = *error_set;
				timeval timeout = { tv_sec, tv_usec };

				int ret = select(max_fd + 1,
					&read_set_copy, nullptr, &error_set_copy,
					ifelse(tv_sec < 0 || tv_usec < 0, nullptr, &timeout));

				if (ret == -1) RuntimeError(get_error_msg());

				for (auto& client : super::clients())
					if (!check_readset(client) || !check_errorset(client))
						return;
			}
		}
	private:
		// 检查是否有可读事件发生
		// 如果有，则调用相应的回调函数
		// 如果有新连接，则调用accept_callback_
		// 如果有数据到达，则调用recv_callback_
		// 如果回调后调用服务器停止函数，则返回false
		bool check_readset(const Socket& fd)
		{
			if (!FD_ISSET(fd, &read_set)) return;

			if (fd == super::socket_.get_socket())
			{
				Socket& client = super::accept();
				accept_callback_(this, client);
				max_fd = std::max<int>(max_fd, client);
			}
			else
				recv_callback_(this, fd, fd.recv());

			return socket_.valid();
		}

		// 检查是否有异常事件发生
		// 如果有则调用error_callback_
		// 如果回调后调用服务器停止函数，则返回false
		bool check_errorset(const Socket& fd)
		{
			if (!FD_ISSET(fd, &error_set)) return;

			int errorno = 0;
			socklen_t len = sizeof(errorno);
			if (fd.getsockopt(SOL_SOCKET, SO_ERROR, &errorno, &len) == -1)
				RuntimeError(get_error_msg());

			error_callback_(this, fd, errorno2str(errorno));
			return socket_.valid();
		}
	private:
		fd_set* read_set, * write_set, * error_set;

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
	};
}
#endif // AYR_SOCKET_TCP_HPP