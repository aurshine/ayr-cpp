#ifndef AYR_NET_SOCKET_HPP
#define AYR_NET_SOCKET_HPP

#include <stdlib.h>

#include "SockAddr.hpp"
#include "startsocket.hpp"
#include "../Atring.hpp"

namespace ayr
{
#if defined(AYR_WIN)

#elif defined(AYR_LINUX)

#define INVALID_SOCKET -1
#define SOCKET_ERROR 0

	def closesocket(int socket) { ::close(socket); }
#endif

	class Socket : public Object<Socket>
	{
	public:
		using self = Socket;

		Socket() : socket_(INVALID_SOCKET) {}

		Socket(int family, int type)
		{
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

		Socket(int socket) : socket_(socket) {}

		Socket(const self& other) : socket_(other.socket_) {}

		self& operator=(const self& other)
		{
			socket_ = other.socket_;
			return *this;
		}

		operator int() const { return socket_; }

		int fd() const { return socket_; }

		void close() const { closesocket(socket_); }

		// 绑定ip:port
		void bind(const CString& ip, int port) const
		{
			SockAddrIn addr(ip, port);
			if (::bind(socket_, addr.get_sockaddr(), addr.get_socklen()) != 0)
				RuntimeError(get_error_msg());
		}

		// 监听端口
		void listen(int backlog = SOMAXCONN) const
		{
			if (::listen(socket_, backlog) != 0)
				RuntimeError(get_error_msg());
		}

		// 接受一个连接
		Socket accept() const
		{
			int accsock = ::accept(socket_, nullptr, nullptr);
			if (accsock == INVALID_SOCKET && errno != EAGAIN && errno != EWOULDBLOCK)
				RuntimeError(get_error_msg());
			return accsock;
		}

		// 连接到ip:port
		void connect(const CString& ip, int port) const
		{
			SockAddrIn addr(ip, port);

			if (::connect(socket_, addr.get_sockaddr(), addr.get_socklen()) != 0)
				RuntimeError(get_error_msg());
		}

		// 发送data，返回已经发送的字节数
		int send(const CString& data, int size = -1, int flags = 0) const
		{
			if (size == -1) size = data.size();
			int num_send = ::send(socket_, data.data(), size, flags);
			if (num_send == -1)
				RuntimeError(get_error_msg());
			return num_send;
		}

		// 参数为Atring的重载
		int send(const Atring& data, int flags = 0) const
		{
			return send(cstr(data), data.byte_size(), flags);
		}

		// 将所有数据连续发送出去，直到发送完毕或出现错误
		void sendall(const CString& data, int size, int flags = 0) const
		{
			// 已经发送出的字节数
			int num_sent = 0;
			const char* ptr = data.data();
			while (num_sent < size)
				num_sent += send(ptr + num_sent, size - num_sent, flags);
		}

		void sendall(const CString& data, int flags = 0) const
		{
			return sendall(data.data(), data.size(), flags);
		}

		void sendall(const Atring& data, int flags = 0) const
		{
			return sendall(cstr(data), data.byte_size(), flags);
		}

		// 将辅助数据和普通数据一起发送
		void sendmsg(const CString& data, int size, int flags = 0) const
		{
			Buffer msg_data(sizeof(u_long) + size);
			int data_size = htonl(size);
			msg_data.append_bytes(&data_size, sizeof(u_long));
			msg_data.append_bytes(data.data(), size);
			sendall(msg_data.data(), msg_data.size(), flags);
		}

		void sendmsg(const CString& data, int flags = 0) const
		{
			sendmsg(data.data(), data.size(), flags);
		}

		void sendmsg(const Atring& data, int flags = 0) const
		{
			sendmsg(cstr(data), data.byte_size(), flags);
		}

		// 发送size个字节的数据到to，数据头部包含了数据大小，需要用recvfrom接收
		int sendto(const CString& data, size_t size, const SockAddrIn& to, int flags = 0) const
		{
			int num_send = ::sendto(socket_, data.data(), size, flags, to.get_sockaddr(), to.get_socklen());
			if (num_send == -1)
				RuntimeError(get_error_msg());
			return num_send;
		}

		int sendto(const CString& data, const SockAddrIn& to, int flags = 0) const
		{
			return sendto(data.data(), data.size(), to, flags);
		}

		int sendto(const Atring& data, const SockAddrIn& to, int flags = 0) const
		{
			return sendto(cstr(data), data.byte_size(), to, flags);
		}

		// 接收最多bufsize个字节的数据
		CString recv(int bufsize, int* length = nullptr, int flags = 0) const
		{
			char* data = ayr_alloc<char>(bufsize);
			int recvd = ::recv(socket_, data, bufsize, flags);
			if (recvd == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
				RuntimeError(get_error_msg());

			if (length) *length = recvd;
			return ostr(data, recvd);
		}

		// 接收所有数据，直到接收完毕或出现错误
		// 只能在非阻塞模式下使用
		CString recvall(int bufsize = 1024, int flags = 0) const
		{
			int length = 0;
			DynArray<CString> datas;
			while (true)
			{
				CString data = recv(bufsize, &length, flags);
				if (length == 0) break;
				datas.append(data);
			}

			return CString::cjoin(datas);
		}

		// 将普通数据和辅助数据一起接收，并返回普通数据
		CString recvmsg(int flags = 0) const
		{
			CString msg_size = recv(sizeof(u_long), nullptr, flags);
			auto msg_size_l = reinterpret_cast<const u_long*>(msg_size.data());
			return recv(ntohl(*msg_size_l), nullptr, flags);
		}

		// 接收sendto发送的数据，如果断开连接，返回空字符串
		// 返回值：数据，来源地址
		std::pair<CString, SockAddrIn> recvfrom(int flags = 0) const
		{
			SockAddrIn from{};
			char* data = ayr_alloc<char>(1024);

			socklen_t addrlen = from.get_socklen();
			::recvfrom(socket_, data, 1024, flags, from.get_sockaddr(), &addrlen);
			return { ostr(data), from };
		}

		// 设置socket为阻塞或非阻塞模式
		void setblocking(bool blocking) const
		{
#if defined(AYR_WIN)
			u_long mode = blocking ? 0 : 1;
			if (::ioctlsocket(socket_, FIONBIO, &mode) != 0)
				RuntimeError(get_error_msg());
#elif defined(AYR_LINUX)
			int flags = fcntl(socket_, F_GETFL, 0);
			if (flags == -1)
				RuntimeError(get_error_msg());
			if (blocking)
				flags &= ~O_NONBLOCK;
			else
				flags |= O_NONBLOCK;
			if (fcntl(socket_, F_SETFL, flags) != 0)
				RuntimeError(get_error_msg());
#endif
		}

		int setsockopt(int level, int optname, const void* optval, socklen_t optlen) const
		{
#if defined(AYR_WIN)
			return ::setsockopt(socket_, level, optname, static_cast<const char*>(optval), optlen);
#elif defined(AYR_LINUX)
			return ::setsockopt(socket_, level, optname, optval, optlen);
#endif
		}

		int getsockopt(int level, int optname, void* optval, socklen_t* optlen) const
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

		// 设置是否复用端口
		void reuse_port(bool on) const
		{
#ifdef SO_REUSEPORT
			int optval = ifelse(on, 1, 0);
			int ret = setsockopt(SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
			if (ret != 0)
				RuntimeError(get_error_msg());
#else
			warn_assert(on, "SO_REUSEPORT not supported on this platform.");
#endif
		}

		// 设置是否复用地址
		void reuse_addr(bool on) const
		{
			int optval = ifelse(on, 1, 0);
			setsockopt(SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
		}

		CString __str__() const { return dstr(std::format("Socket({})", socket_)); }

		cmp_t __cmp__(const self& other) const { return socket_ - other.socket_; }

		cmp_t __cmp__(const int& fd) const { return socket_ - fd; }

		hash_t __hash__() const { return socket_; }

		void __swap__(self& other) { swap(socket_, other.socket_); }
	private:
		int socket_;
	};

	def tcpv4() { return Socket(AF_INET, SOCK_STREAM); }

	def tcpv6() { return Socket(AF_INET6, SOCK_STREAM); }

	def udpv4() { return Socket(AF_INET, SOCK_DGRAM); }

	def udpv6() { return Socket(AF_INET6, SOCK_DGRAM); }
}
#endif