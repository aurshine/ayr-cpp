#ifndef AYR_SOCKET_SERVER_HPP
#define AYR_SOCKET_SERVER_HPP

#include <WinSock2.h>
#include <WS2tcpip.h>

#include <ayr/detail/printer.hpp>

namespace ayr
{
	class Server : public Object<Server>
	{
	public:
		Server()
		{
			WSAData wsa;
			WSAStartup(MAKEWORD(2, 2), &wsa);

			SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		}
	};
}

#endif // AYR_SOCKET_SERVER_HPP