#ifndef AYR_SOCKET_IMPL_HPP
#define AYR_SOCKET_IMPL_HPP

#include <ayr/detail/printer.hpp>
#include <ayr/detail/Buffer.hpp>

namespace ayr
{
#ifdef _WIN32 || _WIN64
#include <ayr/fs/win/winlib.hpp>

#define wsa_init()                                   \
	do {                                              \
		WSADATA wsaData;                               \
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)  \
		{                                                \
			RuntimeError("WSAStartup failed");            \
		}                                                  \
	}while(0)                                               \

#define win_sock_cleanup() WSACleanup();

#else if defined(__linux__) || defined(__unix__)
#include <ayr/fs/linux/linuxlib.hpp>

#define wsa_init()
#define win_sock_cleanup()
#define INVALID_SOCKET -1
#define SOCKET_ERROR 0

	def closesocket(int socket) { ::close(socket); }
#endif

	CString wsa_error_msg()
	{
		int errorno = WSAGetLastError();
		CString error_msg{ 128 };
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, errorno,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), error_msg.data(), 128, nullptr);
		return error_msg;
	}


	struct SockAddrIn : public Object<SockAddrIn>
	{
		SockAddrIn() = default;

		SockAddrIn(const char* ip, int port, int family = AF_INET)
		{
			addr_.sin_family = family;
			addr_.sin_port = htons(port);
			if (inet_pton(AF_INET, ip, &addr_.sin_addr) != 1)
				RuntimeError(wsa_error_msg());
		}

		sockaddr& get_sockaddr() { return *reinterpret_cast<sockaddr*>(&addr_); }

		int get_socklen() { return sizeof(sockaddr_in); }

		CString get_ip(int family = AF_INET)
		{
			CString ip{ 16 };
			if (inet_ntop(family, &addr_.sin_addr, ip.data(), 16) == nullptr)
				RuntimeError(wsa_error_msg());
			return ip;
		}

		int get_port() { return ntohs(addr_.sin_port); }
	private:
		sockaddr_in addr_;
	};


	class Socket : public Object<Socket>
	{
	public:
		Socket(int family, int type)
		{
			wsa_init();

			int protocol = 0;
			switch (type)
			{
			case SOCK_STREAM:
				protocol = IPPROTO_TCP;
				break;
			case SOCK_DGRAM:
				protocol = IPPROTO_UDP;
				break;
			default:
				RuntimeError(wsa_error_msg());
			};

			socket_ = socket(family, type, protocol);
			if (socket_ == INVALID_SOCKET)
				RuntimeError(wsa_error_msg());
		}

		Socket(int socket) : socket_(socket) {}

		~Socket()
		{
			close();
			win_sock_cleanup();
		}

		// 监听ip:port
		void listen(const char* ip, int port, int backlog = SOMAXCONN)
		{
			SockAddrIn addr(ip, port);

			if (::bind(socket_, &addr.get_sockaddr(), addr.get_socklen()) != 0)
				RuntimeError(wsa_error_msg());

			if (::listen(socket_, backlog) != 0)
				RuntimeError(wsa_error_msg());

			print("listen on", ip, ":", port);
		}

		// 接受一个连接
		Socket accept()
		{
			SockAddrIn addr{};
			return ::accept(socket_, &addr.get_sockaddr(), nullptr);
		}

		// 连接到ip:port
		void connect(const char* ip, int port)
		{
			SockAddrIn addr(ip, port);

			if (::connect(socket_, &addr.get_sockaddr(), addr.get_socklen()) != 0)
				RuntimeError(wsa_error_msg());
		}

		// 发送size个字节的数据
		void send(const char* data, int size, int flags = 0)
		{
			Buffer<char> head_data{ 4 + size };
			int head_size = htonl(size);
			head_data.append_bytes(&head_size, 4);
			head_data.append_bytes(data, size);

			send_impl(head_data.data(), size + 4, flags);
		}

		// 返回接收到的一块数据，如果断开连接，返回空字符串
		CString recv(int flags = 0)
		{
			int head_size = 0;
			int recvd = ::recv(socket_, (char*)&head_size, 4, flags);
			if (recvd == SOCKET_ERROR)
				RuntimeError(wsa_error_msg());
			else if (recvd == 0)
				return "";

			return recv_impl(ntohl(head_size), flags);
		}

		void close()
		{
			closesocket(socket_);
			socket_ = INVALID_SOCKET;
		}
	private:
		void send_impl(const char* data, int size, int flags)
		{
			const char* ptr = data;
			int count = size;
			while (count > 0)
			{
				int len = ::send(socket_, ptr, count, flags);
				if (len == SOCKET_ERROR)
					RuntimeError(wsa_error_msg());
				else if (len == 0)
					continue;
				ptr += len;
				count -= len;
			}
		}

		CString recv_impl(int size, int flags)
		{
			CString data{ size };
			int recvd = ::recv(socket_, data, size, flags);
			if (recvd == SOCKET_ERROR)
				RuntimeError(wsa_error_msg());
			return data;
		}
	private:
		int socket_;
	};
}
#endif