#ifndef AYR_SOCKET_IMPL_HPP
#define AYR_SOCKET_IMPL_HPP

#include <stdlib.h>
#include <mutex>

#include "../filesystem.hpp"
#include "../base/Buffer.hpp"
#include "../base/NoCopy.hpp"

namespace ayr
{
#if defined(AYR_WIN)
	def win_startup()
	{
		static bool inited = false;
		static std::mutex init_mutex;

		std::lock_guard<std::mutex> lock(init_mutex);
		if (inited) return;
		inited = true;

		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
			RuntimeError("WSAStartup failed");

		atexit([]() { WSACleanup(); });
	}

#define socket_startup() win_startup()

#elif defined(AYR_LINUX)
#include <cerrno>

#define socket_startup()
#define INVALID_SOCKET -1
#define SOCKET_ERROR 0

	def closesocket(int socket) { ::close(socket); }
#endif


	struct SockAddrIn : public Object<SockAddrIn>
	{
		SockAddrIn() { memset(&addr_, 0, sizeof(sockaddr_in)); };

		SockAddrIn(const char* ip, int port, int family = AF_INET) : SockAddrIn()
		{
			addr_.sin_family = family;
			addr_.sin_port = htons(port);

			if (ip == nullptr)
				addr_.sin_addr.s_addr = INADDR_ANY;
			else if (inet_pton(AF_INET, ip, &addr_.sin_addr) != 1)
				RuntimeError(get_error_msg());
		}

		sockaddr* get_sockaddr() { return reinterpret_cast<sockaddr*>(&addr_); }

		const sockaddr* get_sockaddr() const { return reinterpret_cast<const sockaddr*>(&addr_); }

		int get_socklen() const { return sizeof(sockaddr_in); }

		CString get_ip(int family = AF_INET) const
		{
			CString ip{ 16 };
			if (inet_ntop(family, &addr_.sin_addr, ip.data(), 16) == nullptr)
				RuntimeError(get_error_msg());
			return ip;
		}

		int get_port() const { return ntohs(addr_.sin_port); }

		CString __str__() const { return std::format("{}:{}", get_ip(), get_port()); }
	private:
		sockaddr_in addr_;
	};


	class Socket : public Object<Socket>, public NoCopy
	{
	public:
		Socket(int family, int type)
		{
			socket_startup();

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
				RuntimeError("Invalid socket type");
			};

			socket_ = socket(family, type, protocol);
			if (socket_ == INVALID_SOCKET)
				RuntimeError(get_error_msg());
		}

		Socket(Socket&& other) noexcept : socket_(other.socket_) { other.socket_ = INVALID_SOCKET; }

		Socket(int socket) : socket_(socket) {}

		~Socket() { close(); }

		Socket& operator=(Socket&& other) noexcept
		{
			close();
			socket_ = other.socket_;
			other.socket_ = INVALID_SOCKET;
			return *this;
		}

		operator int() const { return socket_; }

		int get_socket() const { return socket_; }

		// 绑定ip:port
		void bind(const char* ip, int port) const
		{
			SockAddrIn addr(ip, port);

			if (::bind(socket_, addr.get_sockaddr(), addr.get_socklen()) != 0)
				RuntimeError(get_error_msg());
		}

		// 监听端口
		void listen(int backlog = 8) const
		{
			if (::listen(socket_, backlog) != 0)
				RuntimeError(get_error_msg());
		}

		// 接受一个连接
		Socket accept() const
		{
			return ::accept(socket_, nullptr, nullptr);
		}

		// 连接到ip:port
		void connect(const char* ip, int port) const
		{
			SockAddrIn addr(ip, port);

			if (::connect(socket_, addr.get_sockaddr(), addr.get_socklen()) != 0)
				RuntimeError(get_error_msg());
		}

		// 发送size个字节的数据, 数据头部包含了数据大小，需要用recv接收
		void send(const char* data, int size, int flags = 0) const
		{
			Buffer head_data{ 4 + size };
			int head_size = htonl(size);
			head_data.append_bytes(&head_size, 4);
			head_data.append_bytes(data, size);

			send_impl(head_data.data(), size + 4, flags);
		}

		// 接收send发送的数据，如果断开连接，返回空字符串
		CString recv(int flags = 0) const
		{
			int head_size = 0;
			int recvd = ::recv(socket_, (char*)&head_size, 4, flags);
			if (recvd == SOCKET_ERROR)
				RuntimeError(get_error_msg());
			else if (recvd == 0)
				return "";

			return recv_impl(ntohl(head_size), flags);
		}

		// 发送size个字节的数据到to，数据头部包含了数据大小，需要用recvfrom接收
		void sendto(const char* data, size_t size, const SockAddrIn& to, int flags = 0) const
		{
			int num_send = ::sendto(socket_, data, size, flags, to.get_sockaddr(), to.get_socklen());
			if (num_send == SOCKET_ERROR)
				RuntimeError(get_error_msg());
			else if (num_send != size)
				RuntimeError(std::format("Failed to send all data.{}/{}", num_send, size));
		}

		// 接收sendto发送的数据，如果断开连接，返回空字符串
		// 返回值：数据，来源地址
		std::pair<CString, SockAddrIn> recvfrom(int flags = 0) const
		{
			SockAddrIn from{};
			CString data{ 1024 };

			socklen_t addrlen = from.get_socklen();
			::recvfrom(socket_, data.data(), 1024, flags, from.get_sockaddr(), &addrlen);
			return { data, from };
		}

		int setsockopt(int level, int optname, const void* optval, int optlen) const
		{
#if defined(AYR_WIN)
			return ::setsockopt(socket_, level, optname, static_cast<const char*>(optval), optlen);
#elif defined(AYR_LINUX)
			return ::setsockopt(socket_, level, optname, optval, optlen);
#endif
		}

		int getsockopt(int level, int optname, void* optval, int* optlen) const
		{
#if defined(AYR_WIN)
			return ::getsockopt(socket_, level, optname, static_cast<char*>(optval), optlen);
#elif defined(AYR_LINUX)
			return ::getsockopt(socket_, level, optname, optval, optlen);
#endif
		}

		void setbuffer(int size, CString mode) const
		{
			if (mode == "r")
				setsockopt(SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
			else if (mode == "w")
				setsockopt(SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
			else
				ValueError(std::format("Invalid buffer mode {}. Should be 'r' or 'w'.", mode));
		}

		bool valid() const { return socket_ != INVALID_SOCKET; }

		void close()
		{
			closesocket(socket_);
			socket_ = INVALID_SOCKET;
		}

		CString __str__() const { return std::format("Socket({})", socket_); }
	private:
		// 发送size个字节的数据
		void send_impl(const char* data, int size, int flags) const
		{
			const char* ptr = data;
			while (size > 0)
			{
				int num_send = ::send(socket_, ptr, size, flags);
				if (num_send == SOCKET_ERROR)
					RuntimeError(get_error_msg());
				else if (num_send == 0)
					continue;
				ptr += num_send;
				size -= num_send;
			}
		}

		// 接收size个字节的数据
		CString recv_impl(int size, int flags) const
		{
			CString data{ size };
			char* ptr = data.data();
			while (size > 0)
			{
				int recvd = ::recv(socket_, ptr, size, flags);
				if (recvd == SOCKET_ERROR)
					RuntimeError(get_error_msg());
				ptr += recvd;
				size -= recvd;
			}

			return data;
		}
	private:
		int socket_;
	};
}
#endif