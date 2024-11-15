#ifndef AYR_SOCKET_UDP_HPP_
#define AYR_SOCKET_UDP_HPP_

#include <vector>

#include "Socket.hpp"

namespace ayr
{
	class UdpServer : public Object<UdpServer>
	{
	public:
		UdpServer(const char* ip, int port, int famliy = AF_INET) : socket_(famliy, SOCK_DGRAM)
		{
			socket_.bind(ip, port);
		}

		void send(const char* data, int size, const SockAddrIn& to, int flags = 0) const { socket_.sendto(data, size, to, flags); }

		std::pair<CString, SockAddrIn> recv(int flags = 0) const { return socket_.recvfrom(flags); }
	private:
		Socket socket_;
	};

	class UdpClient : public Object<UdpClient>
	{
	public:
		UdpClient(int famliy = AF_INET) : socket_(famliy, SOCK_DGRAM) {}

		UdpClient(const char* serever_ip, int server_port, int famliy = AF_INET) :
			socket_(famliy, SOCK_DGRAM), server_addr_(serever_ip, server_port) {}

		void send(const char* data, int size, int flags = 0) const { socket_.sendto(data, size, server_addr_, flags); }

		std::pair<CString, SockAddrIn> recv(int flags = 0) const { return socket_.recvfrom(flags); }

		~UdpClient() { send("", 0); }
	private:
		Socket socket_;

		SockAddrIn server_addr_;
	};
}
#endif