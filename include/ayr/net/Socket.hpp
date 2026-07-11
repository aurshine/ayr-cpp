#ifndef AYR_NET_SOCKET_HPP
#define AYR_NET_SOCKET_HPP

#include "utils.hpp"
#include "../coro/IoContext.hpp"


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
		class Socket
		{
			using self = Socket;

			BaseSocket fd_;

			// TLS members
			SSL_CTX* ssl_ctx_;

			SSL* ssl_;

			coro::IoContext* io_context_;
		public:
			Socket(BaseSocket fd, coro::IoContext* io_context, SSL_CTX* ctx = nullptr) :
				fd_(fd),
				ssl_ctx_(ctx),
				ssl_(nullptr),
				io_context_(io_context)
			{
				setblocking(fd_, false);
				if (ssl_ctx_)
				{
					ssl_ = SSL_new(ssl_ctx_);
					if (!ssl_)
						RuntimeError("Failed to create SSL object.");

					SSL_set_fd(ssl_, fd_);
				}
			}

			Socket(self&& other) noexcept :
				fd_(other.fd_),
				ssl_ctx_(other.ssl_ctx_),
				ssl_(other.ssl_),
				io_context_(other.io_context_)
			{
				other.fd_ = -1;
				other.ssl_ctx_ = nullptr;
				other.ssl_ = nullptr;
				other.io_context_ = nullptr;
			}

			~Socket()
			{
				if (ssl_)
				{
					SSL_shutdown(ssl_);
					SSL_free(ssl_);
					ssl_ = nullptr;
				}

				if (valid())
				{
					io_context_->close(this->fd_);
					fd_ = -1;
				}
			}

			self& operator=(self&& other) noexcept
			{
				if (this == &other) return *this;
				ayr_destroy(this);
				return *ayr_construct(this, std::move(other));
			}

			// 判断文件描述符是否有效
			bool valid() const { return fd_ != -1; }

			/*
			* @brief ssl握手
			*
			* @param is_server 是否为服务端
			*/
			coro::Task<void> handshake(bool is_server)
			{
				if (ssl_ == nullptr) co_return;

				if (is_server)
					SSL_set_accept_state(ssl_);
				else
					SSL_set_connect_state(ssl_);

				while (true)
				{
					int ret = ifelse(is_server, SSL_accept(ssl_), SSL_connect(ssl_));
					if (ret == 1) co_return;

					// co_await ssl_eagain_wait(ret);
				}
			}

			/*
			* @brief 协程挂起，直到socket写完buffer
			*
			* @param buffer 要写入的数据
			*
			* @return 返回写入结果，true表示成功，false表示对方关闭连接
			*/
			coro::Task<bool> write(Buffer& buffer)
			{
				while (buffer.readable_size() > 0)
				{
					auto result = co_await io_context_->write_awaiter(fd_, &buffer);
					if (!result.ok() || result.bytes == 0)
						co_return false;
				}
				co_return true;
			}

			/*
			* @brief 协程挂起，直到读取到数据
			*
			* @param buffer 要读取的数据存放的buffer
			*
			* @return 返回读取的字节数, -1表示读取错误, 0表示对方关闭连接
			*/
			coro::ReadWaiter read(Buffer& buffer)
			{
				return io_context_->read_awaiter(fd_, &buffer);
			}

			constexpr std::strong_ordering operator<=>(const self& other) const { return fd_ <=> other.fd_; }

			constexpr bool operator==(const self& other) const { return fd_ == other.fd_; }

			hash_t __hash__() const { return fd_; }

			void __repr__(Buffer& buffer) const { buffer << "Socket(" << fd_ << ")"; }
		};


		/*
		* @brief 用于监听窗口的类
		*/
		class Acceptor
		{
			using self = Acceptor;

			int family_;

			BaseSocket fd_;

			coro::IoContext* io_context_;

			SSL_CTX* ssl_ctx_;
		public:
			Acceptor(const CString& ip, int port, coro::IoContext* io_context, int family = AF_INET, SSL_CTX* ssl_ctx = nullptr) :
				family_(family),
				fd_(net::socket(family, SOCK_STREAM, IPPROTO_TCP)),
				io_context_(io_context),
				ssl_ctx_(ssl_ctx)
			{
				net::reuse_addr(fd_, true);

				if (family == AF_INET)
				{
					sockaddr_in addr;
					std::memset(&addr, 0, sizeof(addr));
					addr.sin_family = family_;
					addr.sin_port = htons(port);
					if (inet_pton(addr.sin_family, ip.c_str(), &addr.sin_addr) != 1)
						RuntimeError("Invalid host address.");
					if (::bind(fd_, (sockaddr*)&addr, sizeof(addr)) != 0)
						RuntimeError(get_error_msg());
				}
				else if (family == AF_INET6)
				{
					sockaddr_in6 addr;
					std::memset(&addr, 0, sizeof(addr));
					addr.sin6_family = family_;
					addr.sin6_port = htons(port);
					if (inet_pton(addr.sin6_family, ip.c_str(), &addr.sin6_addr) != 1)
						RuntimeError(format("Invalid host address. {}", get_error_msg()));
					if (::bind(fd_, (sockaddr*)&addr, sizeof(addr)) != 0)
						RuntimeError(get_error_msg());
				}
				

				net::setblocking(fd_, false);
			}

			Acceptor(self&& other) noexcept :
				fd_(other.fd_),
				io_context_(other.io_context_),
				ssl_ctx_(other.ssl_ctx_)
			{
				other.fd_ = -1;
				other.io_context_ = nullptr;
				other.ssl_ctx_ = nullptr;
			}

			~Acceptor()
			{
				if (valid())
				{
					io_context_->close(fd_);
					fd_ = -1;
				}
			}

			self& operator=(self&& other) noexcept
			{
				if (this == &other) return *this;
				return *ayr_construct(this, std::move(other));
			}

			bool valid() const { return fd_ != -1; }

			BaseSocket fd() const { return fd_; }

			void listen(int backlog = SOMAXCONN) const
			{
				if (::listen(fd_, backlog) != 0)
					RuntimeError(get_error_msg());
			}

			coro::Task<Socket> accept()
			{
				auto result = co_await io_context_->accept_awaiter(fd_, family_);
				
				if (!result.ok())
					RuntimeError(errorno2str(result.error));
				
				Socket sock(result.socket, io_context_, ssl_ctx_);
				co_await sock.handshake(true);
				co_return sock;
			}

			constexpr std::strong_ordering operator<=>(const self& other) const { return fd_ <=> other.fd_; }

			constexpr bool operator==(const self& other) const { return fd_ == other.fd_; }

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
		def open_connect(const CString& host, int port, coro::IoContext* io_context, SSL_CTX* ssl_ctx = nullptr) -> coro::Task<Socket>
		{
			addrinfo hints, * res = nullptr;
			memset(&hints, 0, sizeof(hints));

			// 尝试IPv4和IPv6
			hints.ai_family = AF_UNSPEC;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = IPPROTO_TCP;

			if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res) == 0)
			{
				exitask([&res] { freeaddrinfo(res); });

				for (addrinfo* p = res; p; p = p->ai_next)
				{
					BaseSocket fd = net::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
					auto result = co_await io_context->connect_awaiter(fd, p);
					if (result.ok())
					{
						Socket sock(fd, io_context, ssl_ctx);
						co_await sock.handshake(false);
						co_return sock;
					}
					net::close(fd);
				}
			}

			RuntimeError("Failed to connect to server.");
			co_return Socket(-1, io_context);
		}
	}
}
#endif // AYR_NET_SOCKET_HPP