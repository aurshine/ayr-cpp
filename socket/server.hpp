#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "../law/law.hpp"

namespace ayr
{
	class Server : public Object
	{
	public:
		Server()
		{
			WSAData wsa;
			WSAStartup(MAKEWORD(2, 2), &wsa);

			int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		}
	};
}