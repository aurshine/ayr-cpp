#ifndef AYR_SOCKET_SERVER_HPP
#define AYR_SOCKET_SERVER_HPP

#include "socket_impl.hpp"

namespace ayr
{
	class Server : public SocketImpl
	{
		using self = Server;

		using super = SocketImpl;
	public:
		Server(const CString& ip, int port) : super(AF_INET, SOCK_STREAM, IPPROTO_TCP)
		{
			sockaddr_in serverAddr;
			serverAddr.sin_family = AF_INET;
			serverAddr.sin_port = htons(port);
			serverAddr.sin_addr.s_addr = inet_addr(ip);

			if (bind(socket_, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
				RuntimeError("bind failed");

			if (listen(socket_, SOMAXCONN) == SOCKET_ERROR)
				RuntimeError("listen failed");
		}

		void accept()
		{
			sockaddr_in client_addr;
			SOCKET client_socket = ::accept(socket_, (sockaddr*)&client_addr, nullptr);
			if (client_socket == INVALID_SOCKET)
				RuntimeError("accept failed");


		}
	};
}

#endif // AYR_SOCKET_SERVER_HPP