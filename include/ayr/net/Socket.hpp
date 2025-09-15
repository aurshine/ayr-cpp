#ifndef AYR_NET_SOCKET_HPP
#define AYR_NET_SOCKET_HPP

#include <stdlib.h>

#include "utils.hpp"
#include "../Atring.hpp"


namespace ayr
{
	namespace net
	{
		/*
		* @brief Socket类，封装了socket相关的操作
		*
		* @note 该类不负责创建socket，只负责对socket进行操作
		*
		* 每个传入的socket都被设置为非阻塞模式
		*/
		class Socket : public Object<Socket>
		{
			using self = Socket;

			using super = Object<Socket>;

			int fd_;

			coro::EventAwaiter read_awaiter_;

			coro::EventAwaiter write_awaiter_;
		public:
			Socket(int fd, coro::IoContext* io_context) :
				fd_(fd),
				read_awaiter_(io_context->wait_for_read(fd_)),
				write_awaiter_(io_context->wait_for_write(fd_))
			{
				setblocking(fd_, false);
			}

			Socket(int fd) : Socket(fd, &coro::asyncio) {}

			Socket(self&& other) noexcept :
				fd_(other.fd_),
				read_awaiter_(std::move(other.read_awaiter_)),
				write_awaiter_(std::move(other.write_awaiter_))
			{
				other.fd_ = -1;
			}

			~Socket()
			{
				if (valid())
				{
					net::close(fd_);
					fd_ = -1;
				}
			}

			self& operator=(self&& other) noexcept
			{
				if (this == &other) return *this;
				return *ayr_construct(this, std::move(other));
			}

			operator int() const { return fd_; }

			// 判断文件描述符是否有效
			bool valid() const { return fd_ != -1; }

			// 返回文件描述符
			int fd() const { return fd_; }

			coro::Task<int> write(const CString& data, int flags = 0)
			{
				int num_write = net::write(fd_, data, flags);
				// 调用net::write返回-1表示非阻塞模式下没有空间可写，需要挂起等待
				if (num_write != -1) co_return num_write;
				co_await write_awaiter_;
				co_return net::write(fd_, data, flags);
			}

			// 协程挂起，直到socket可读
			coro::Task<int> write(Buffer& buffer, int flags = 0)
			{
				int num_write = net::write(fd_, buffer, flags);
				// 调用net::write返回-1表示非阻塞模式下没有空间可写，需要挂起等待
				if (num_write != -1) co_return num_write;
				co_await write_awaiter_;
				co_return net::write(fd_, buffer, flags);
			}

			// 协程挂起，直到socket可写
			coro::Task<int> read(Buffer& buffer, int flags = 0)
			{
				int num_read = net::read(fd_, buffer, flags);
				// 调用net::read返回-1表示非阻塞模式下没有数据可读，需要挂起等待
				if (num_read != -1) co_return num_read;
				co_await read_awaiter_;
				co_return net::read(fd_, buffer, flags);
			}

			cmp_t __cmp__(const self& other) const { return fd_ - other.fd_; }

			cmp_t __cmp__(const int& fd) const { return fd_ - fd; }

			hash_t __hash__() const { return fd_; }

			void __repr__(Buffer& buffer) const { buffer << "Socket(" << fd_ << ")"; }
		};


		/*
		* @brief 用于监听窗口的类
		*/
		class Acceptor : public Object<Acceptor>
		{
			using self = Acceptor;

			using super = Object<Acceptor>;

			int fd_;

			coro::IoContext* io_context_;

			coro::EventAwaiter read_awaiter_;
		public:
			Acceptor(const CString& ip, int port, coro::IoContext* io_context, bool ipv6 = false) :
				fd_(net::socket(ifelse(ipv6, AF_INET6, AF_INET), SOCK_STREAM, IPPROTO_TCP)),
				read_awaiter_(io_context->wait_for_read(fd_)),
				io_context_(io_context)
			{
				sockaddr_in addr;
				std::memset(&addr, 0, sizeof(addr));
				addr.sin_family = ifelse(ipv6, AF_INET6, AF_INET);
				addr.sin_port = htons(port);
				if (inet_pton(addr.sin_family, ip.c_str().c_str(), &addr.sin_addr) != 1)
					RuntimeError("Invalid host address.");
				if (::bind(fd_, (sockaddr*)&addr, sizeof(addr)) != 0)
					RuntimeError(get_error_msg());

				net::setblocking(fd_, false);
			}

			Acceptor(const CString& ip, int port, bool ipv6 = false) : Acceptor(ip, port, &coro::asyncio, ipv6) {}

			Acceptor(self&& other) noexcept :
				fd_(other.fd_),
				read_awaiter_(std::move(other.read_awaiter_)),
				io_context_(other.io_context_)
			{
				other.fd_ = -1;
				other.io_context_ = nullptr;
			}

			~Acceptor() { close(fd_); }

			self& operator=(self&& other) noexcept
			{
				if (this == &other) return *this;
				return *ayr_construct(this, std::move(other));
			}

			int fd() const { return fd_; }

			void listen(int backlog = SOMAXCONN) const
			{
				if (::listen(fd_, backlog) != 0)
					RuntimeError(get_error_msg());
			}

			coro::Task<Socket> accept()
			{
				int fd = ::accept(fd_, nullptr, nullptr);
				if (fd == -1)
				{
					if (!is_eagain())
						RuntimeError(get_error_msg());
					co_await read_awaiter_;
					fd = ::accept(fd_, nullptr, nullptr);
					if (fd == -1)
						RuntimeError(get_error_msg());
				}
				co_return Socket(fd, io_context_);
			}

			cmp_t __cmp__(const self& other) const { return fd_ - other.fd_; }

			hash_t __hash__() const { return fd_; }

			void __repr__(Buffer& buffer) const { buffer << "Acceptor(" << fd_ << ")"; }
		};

		/*
		* @brief 提供主机名和端口，连接到服务器
		*
		* @param host 主机域名或IP地址
		*
		* @param port 端口号
		*
		* @return 返回连接成功的文件描述符，否则抛出异常
		*/
		def open_connect(const CString& host, int port, coro::IoContext* io_context = nullptr) -> coro::Task<Socket>
		{
			if (io_context == nullptr)
				io_context = &coro::asyncio;
			addrinfo hints, * res = nullptr;
			memset(&hints, 0, sizeof(hints));

			// 尝试IPv4和IPv6
			hints.ai_family = AF_UNSPEC;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = IPPROTO_TCP;

			if (getaddrinfo(host.c_str().c_str(), std::to_string(port).c_str(), &hints, &res) == 0)
			{
				ExTask exit([&res] { freeaddrinfo(res); });

				for (addrinfo* p = res; p; p = p->ai_next)
				{
					int fd = net::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
					if (co_await co_connect(fd, p->ai_addr, p->ai_addrlen, io_context))
						co_return Socket(fd, io_context);
					net::close(fd);
				}
			}

			RuntimeError("Failed to connect to server.");
			co_return Socket(-1, io_context);
		}
#if defined(AYR_WIN)
		// 用于初始化Winsock的类
		class _StartSocket : public Object<_StartSocket>
		{
		public:
			_StartSocket()
			{
				WSADATA wsaData;
				if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
					RuntimeError("WSAStartup failed");
			}

			~_StartSocket() { WSACleanup(); }

		};

		static const _StartSocket __startsocket;
	}
#endif // AYR_WIN
}
#endif // AYR_NET_SOCKET_HPP