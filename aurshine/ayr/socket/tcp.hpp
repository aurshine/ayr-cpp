#ifndef AYR_SOCKET_TCP_HPP
#define AYR_SOCKET_TCP_HPP

#include <vector>

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
	private:
		Socket socket_;

		std::vector<Socket> clients_;
	};

	class TcpClient : public Object<TcpClient>
	{
	public:
		TcpClient(const char* ip, int port, int family = AF_INET) : socket_(family, SOCK_STREAM)
		{
			socket_.connect(ip, port);
		}

		void send(const char* data, int size, int flags = 0) const { socket_.send(data, size, flags); }

		CString recv(int flags = 0) const { return socket_.recv(flags); }
	private:
		Socket socket_;
	};
}

#endif // AYR_SOCKET_TCP_HPP