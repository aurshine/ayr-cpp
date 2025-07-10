#ifndef AYR_NET_SOCKET_HPP
#define AYR_NET_SOCKET_HPP

#include <stdlib.h>

#include "SockAddr.hpp"
#include "startsocket.hpp"
#include "../base/ExTask.hpp"
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
		using self = Socket;
	public:
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

		// 判断socket是否有效
		bool valid() const { return socket_ != INVALID_SOCKET; }

		// 返回文件描述符
		int fd() const { return socket_; }

		// 关闭socket
		void close() const { closesocket(socket_); }

		/*
		* @brief 绑定ip:port
		*
		* @param ip 绑定的ip地址
		*
		* @param port 绑定的端口
		*/
		void bind(const CString& ip, int port) const
		{
			SockAddrIn addr(ip, port);
			if (::bind(socket_, addr.get_sockaddr(), addr.get_socklen()) != 0)
				RuntimeError(get_error_msg());
		}

		/*
		* @brief 监听socket
		*
		* @param backlog 最大连接数
		*/
		void listen(int backlog = SOMAXCONN) const
		{
			if (::listen(socket_, backlog) != 0)
				RuntimeError(get_error_msg());
		}

		/*
		* @brief 接受连接
		*
		* @return 返回一个新的Socket对象，用于与客户端通信
		*
		* @note 非阻塞模式下可能会返回INVALID_SOCKET
		*/
		Socket accept() const
		{
			int accsock = ::accept(socket_, nullptr, nullptr);
			if (accsock == INVALID_SOCKET && errno != EAGAIN && errno != EWOULDBLOCK)
				RuntimeError(get_error_msg());
			return accsock;
		}

		/*
		* @brief 连接到ip:port
		*
		* @param ip 要连接的ip地址
		*
		* @param port 要连接的端口
		*/
		void connect(const CString& ip, int port) const
		{
			SockAddrIn addr(ip, port);

			if (::connect(socket_, addr.get_sockaddr(), addr.get_socklen()) != 0)
				RuntimeError(get_error_msg());
		}

		/*
		* @brief 发送数据到socket
		*
		* @param data 要发送的数据
		*
		* @param size 要发送的数据长度，默认为-1，表示发送整个data
		*
		* @param flags 发送标志, 默认为0
		*
		* @return 实际发送的字节数
		*/
		int send(const CString& data, int size = -1, int flags = 0) const
		{
			if (size == -1) size = data.size();
			int num_send = ::send(socket_, data.data(), size, flags);
			if (num_send == -1)
				RuntimeError(get_error_msg());
			return num_send;
		}

		/*
		* @brief 将所有数据连续发送出去，直到发送完毕或出现错误
		*
		* @param data 要发送的数据
		*
		* @param size 要发送的数据长度, 默认为-1，表示发送整个data
		*
		* @param flags 发送标志, 默认为0
		*/
		void sendall(const CString& data, int size = -1, int flags = 0) const
		{
			if (size == -1) size = data.size();
			// 已经发送出的字节数
			int num_sent = 0;
			const char* ptr = data.data();
			while (num_sent < size)
				num_sent += send(ptr + num_sent, size - num_sent, flags);
		}

		/*
		* @brief 将辅助数据和普通数据一起发送
		*
		* @detail 发送的前四个字节是数据大小，后面是数据
		*
		* @param data 要发送的数据
		*
		* @param size 要发送的数据长度, 默认为-1，表示发送整个data
		*
		* @param flags 发送标志, 默认为0
		*
		* @note 可以使用recvmsg接收数据
		*/
		void sendmsg(const CString& data, int size = -1, int flags = 0) const
		{
			if (size == -1) size = data.size();
			Buffer msg_data(4 + size);
			int data_size = htonl(size);
			msg_data.append_bytes(&data_size, 4);
			msg_data.append_bytes(data.data(), size);
			sendall(msg_data.data(), msg_data.size(), flags);
		}

		/*
		* @brief 发送size个字节的数据到to，数据头部包含了数据大小，需要用recvfrom接收
		*
		* @param data 要发送的数据
		*
		* @param size 要发送的数据长度
		*
		* @param to 要发送到的地址
		*
		* @param flags 发送标志, 默认为0
		*
		* @return 实际发送的字节数
		*/
		int sendto(const CString& data, size_t size = -1, const SockAddrIn& to, int flags = 0) const
		{
			if (size == -1) size = data.size();
			int num_send = ::sendto(socket_, data.data(), size, flags, to.get_sockaddr(), to.get_socklen());
			if (num_send == -1)
				RuntimeError(get_error_msg());
			return num_send;
		}

		/*
		* @brief 接收最多bufsize个字节的数据
		*
		* @param bufsize 要接收的最大字节数
		*
		* @param length 实际接收的字节数，如果为nullptr则不写入
		*
		* @param flags 接收标志, 默认为0
		*
		* @return 接收到的数据
		*/
		CString recv(int bufsize = 1024, int* length = nullptr, int flags = 0) const
		{
			char* data = ayr_alloc<char>(bufsize);
			int recvd = ::recv(socket_, data, bufsize, flags);
			if (recvd == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
				RuntimeError(get_error_msg());

			if (length) *length = recvd;
			return ostr(data, recvd);
		}

		/*
		* @brief 接收所有数据，直到接收完毕或出现错误
		*
		* @param bufsize 要接收的最大字节数
		*
		* @param flags 接收标志, 默认为0
		*
		* @return 接收到的数据
		*
		* @note 阻塞模式会阻塞直到断开连接
		*/
		CString recvall(int bufsize = 1024, int flags = 0) const
		{
			int length = 0;
			DynArray<CString> datas;
			while (true)
			{
				CString data = recv(bufsize, &length, flags);
				if (length == 0) break;
				datas.append(std::move(data));
			}

			return CString::cjoin(datas);
		}

		/*
		* @brief 将普通数据和辅助数据一起接收，并返回普通数据
		*
		* @param flags 接收标志, 默认为0
		*/
		CString recvmsg(int flags = 0) const
		{
			// 需要接收到的字节数
			int num_need = 4;
			char msg_size[4];
			char* ptr = msg_size;
			while (num_need)
			{
				int length = 0;
				CString tmp = recv(num_need, &length, flags);
				std::memcpy(ptr, tmp.data(), length);
				num_need -= length;
				ptr += length;
			}
			auto msg_size_l = reinterpret_cast<const u_long*>(msg_size);
			return recv(ntohl(*msg_size_l), nullptr, flags);
		}

		/*
		* @brief 接收sendto发送的数据
		*
		* @param bufsize 要接收的最大字节数
		*
		* @param flags 接收标志, 默认为0
		*
		* @return 接收到的数据和来源地址
		*/
		std::pair<CString, SockAddrIn> recvfrom(int bufsize = 1024, int flags = 0) const
		{
			SockAddrIn from{};
			char* data = ayr_alloc<char>(bufsize);

			socklen_t addrlen = from.get_socklen();
			::recvfrom(socket_, data, bufsize, flags, from.get_sockaddr(), &addrlen);
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

		/*
		* @brief 设置socket选项
		*
		* @param level 选项级别
		*
		* @param optname 选项名称
		*
		* @param optval 选项值
		*
		* @param optlen 选项长度
		*/
		int setsockopt(int level, int optname, const void* optval, socklen_t optlen) const
		{
#if defined(AYR_WIN)
			return ::setsockopt(socket_, level, optname, static_cast<const char*>(optval), optlen);
#elif defined(AYR_LINUX)
			return ::setsockopt(socket_, level, optname, optval, optlen);
#endif
		}

		/*
		* @brief 获取socket选项
		*
		* @param level 选项级别
		*
		* @param optname 选项名称
		*
		* @param optval 选项值
		*
		* @param optlen 选项长度
		*/
		int getsockopt(int level, int optname, void* optval, socklen_t* optlen) const
		{
#if defined(AYR_WIN)
			return ::getsockopt(socket_, level, optname, static_cast<char*>(optval), optlen);
#elif defined(AYR_LINUX)
			return ::getsockopt(socket_, level, optname, optval, optlen);
#endif
		}

		/*
		* @brief 设置缓冲区大小
		*
		* @param size 缓冲区大小
		*
		* @param mode 缓冲区模式，'r'表示接收缓冲区，'w'表示发送缓冲区
		*/
		void setbuffer(int size, const CString& mode) const
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


	/*
	* @brief 提供主机地址和端口，连接到服务器
	*
	* @param host 主机域名或IP地址
	*
	* @param port 端口号
	*
	* @return 返回连接成功的字符串，否则抛出异常
	*/
	def host_connect(const CString& host, const CString& port)
	{
		addrinfo hints, * res = nullptr;
		memset(&hints, 0, sizeof(hints));

		// 尝试IPv4和IPv6
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		if (getaddrinfo(host.c_str().c_str(), port.c_str().c_str(), &hints, &res) == 0)
		{
			ExTask exit([&res] { freeaddrinfo(res); });

			for (addrinfo* p = res; p; p = p->ai_next)
			{
				Socket sock(p->ai_family, p->ai_socktype);
				if (::connect(sock.fd(), p->ai_addr, p->ai_addrlen) == 0)
					return sock;
				sock.close();
			}
		}

		RuntimeError("Failed to connect to server.");
	}

	def tcpv4() { return Socket(AF_INET, SOCK_STREAM); }

	def tcpv6() { return Socket(AF_INET6, SOCK_STREAM); }

	def udpv4() { return Socket(AF_INET, SOCK_DGRAM); }

	def udpv6() { return Socket(AF_INET6, SOCK_DGRAM); }
}
#endif