#ifndef AYR_SOCKET_CLIENT_HPP_
#define AYR_SOCKET_CLIENT_HPP_

#include "socket_impl.hpp"

namespace ayr
{
	class Client : public SocketImpl
	{
		using self = Client;

		using super = SocketImpl;
	public:
		Client(const CString& ip, int port) : super(AF_INET, SOCK_STREAM, IPPROTO_TCP)
		{
			sockaddr_in serverAddr;
			serverAddr.sin_family = AF_INET;
			serverAddr.sin_port = htons(port);
			serverAddr.sin_addr.s_addr = inet_addr(ip);

			if (connect(socket_, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
				RuntimeError("connect failed");
		}
	};
}
#endif // AYR_SOCKET_CLIENT_HPP_