#ifndef AYR_SOCKET_IMPL_HPP
#define AYR_SOCKET_IMPL_HPP

#include <WinSock2.h>
#include <WS2tcpip.h>

#include <ayr/detail/printer.hpp>

namespace ayr
{
	def win_sock_init()
	{
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
			RuntimeError("WSAStartup failed");
	}

	def win_sock_cleanup()
	{
		WSACleanup();
	}

	class SocketImpl : public Object<SocketImpl>
	{
	public:
		SocketImpl(int domain, int type, int protocol)
		{
			win_sock_init();
			socket_ = socket(domain, type, protocol);
			if (socket_ == INVALID_SOCKET)
				RuntimeError("socket failed");
		}

		~SocketImpl()
		{
			closesocket(socket_);
			win_sock_cleanup();
		}

		void send(const CString& message)
		{
			int len = ::send(socket_, message, message.size(), 0);
			if (len == SOCKET_ERROR)
				RuntimeError("send failed");
		}

		CString recv()
		{
			CString message{ 1024 };
			int len = ::recv(socket_, message.data(), message.size(), 0);
			if (len == SOCKET_ERROR)
				RuntimeError("recv failed");
			return message;
		}
	protected:
		SOCKET socket_;
	};
}
#endif