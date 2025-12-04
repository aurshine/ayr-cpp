#ifndef AYR_NET_SOCKET_HPP
#define AYR_NET_SOCKET_HPP

#include "utils.hpp"

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

			int read_fd_, write_fd_;

			coro::EventAwaiter read_awaiter_;

			coro::EventAwaiter write_awaiter_;

			// TLS members
			SSL_CTX* ssl_ctx_;

			SSL* ssl_;
		public:
			Socket(int fd, coro::IoContext* io_context, SSL_CTX* ctx = nullptr) :
				read_fd_(fd),
				write_fd_(net::dup(fd)),
				read_awaiter_(io_context->wait_for_read(read_fd_)),
				write_awaiter_(io_context->wait_for_write(write_fd_)),
				ssl_ctx_(ctx),
				ssl_(nullptr)
			{
				setblocking(fd, false);
				if (ssl_ctx_)
				{
					ssl_ = SSL_new(ssl_ctx_);
					if (!ssl_)
						RuntimeError("Failed to create SSL object.");

					SSL_set_fd(ssl_, fd);
				}
			}

			Socket(self&& other) noexcept :
				read_fd_(other.read_fd_),
				write_fd_(other.write_fd_),
				read_awaiter_(std::move(other.read_awaiter_)),
				write_awaiter_(std::move(other.write_awaiter_)),
				ssl_ctx_(other.ssl_ctx_),
				ssl_(other.ssl_)
			{
				other.read_fd_ = -1;
				other.write_fd_ = -1;
				other.ssl_ctx_ = nullptr;
				other.ssl_ = nullptr;
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
					net::close(read_fd_);
					net::close(write_fd_);
					read_fd_ = -1;
					write_fd_ = -1;
				}
			}

			self& operator=(self&& other) noexcept
			{
				if (this == &other) return *this;
				return *ayr_construct(this, std::move(other));
			}

			// 判断文件描述符是否有效
			bool valid() const { return read_fd_ != -1 || write_fd_ != -1; }

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

					co_await ssl_eagain_wait(ret);
				}
			}

			/*
			* @brief 协程挂起，直到socket写完data
			*
			* @param data 要写入的数据
			*
			* @return 返回写入结果，true表示成功，false表示对方关闭连接
			*/
			coro::Task<bool> write(const CString& data, int flags = 0)
			{
				c_size data_written = 0, data_size = data.size();
				while (data_written < data_size)
				{
					int num_written = co_await ifelse(ssl_, write_ssl_once(data), write_fd_once(data));
					if (num_written == 0)
						co_return false;
					data_written += num_written;
				}

				co_return true;
			}

			/*
			* @brief 协程挂起，直到socket写完buffer
			*
			* @param buffer 要写入的数据
			*
			* @return 返回写入结果，true表示成功，false表示对方关闭连接
			*/
			coro::Task<bool> write(Buffer& buffer, int flags = 0)
			{
				while (buffer.readable_size() > 0)
				{
					int num_written = co_await ifelse(ssl_, write_ssl_once(buffer), write_fd_once(buffer));
					if (num_written == 0)
						co_return false;
				}
				co_return true;
			}

			/*
			* @brief 协程挂起，直到读取到数据
			*
			* @param buffer 要读取的数据存放的buffer
			*
			* @param read_size 要读取的数据大小，-1表示读取整个buffer大小
			*
			* @return 返回读取的字节数, -1表示读取错误, 0表示对方关闭连接
			*/
			coro::Task<int> read(Buffer& buffer, c_size read_size = 1024, int flags = 0)
			{
				while (true)
				{
					if (ssl_)
					{
						int num_read = net::read(ssl_, buffer, read_size);
						if (num_read == -1)
							co_await ssl_eagain_wait(num_read);
						else
							co_return num_read;
					}
					else
					{
						int num_read = net::read(read_fd_, buffer, read_size, flags);
						if (num_read == -1)
							co_await read_awaiter_;
						else
							co_return num_read;
					}
				}
			}

			constexpr std::strong_ordering operator<=>(const self& other) const { return read_fd_ <=> other.read_fd_; }

			constexpr bool operator==(const self& other) const { return read_fd_ == other.read_fd_; }

			hash_t __hash__() const
			{
				int bytes[2] = { read_fd_, write_fd_ };
				return bytes_hash(reinterpret_cast<const char*>(bytes), sizeof(bytes));
			}

			void __repr__(Buffer& buffer) const { buffer << "Socket(" << read_fd_ << ")"; }
		private:
			/*
			* @brief 等待ssl需要的读写事件
			*
			* @param ret 之前的ssl函数返回值
			*/
			coro::Task<void> ssl_eagain_wait(int ret)
			{
				int err = SSL_get_error(ssl_, ret);
				if (err == SSL_ERROR_WANT_READ)
					co_await read_awaiter_;
				else if (err == SSL_ERROR_WANT_WRITE)
					co_await write_awaiter_;
				else
					SSLError(ssl_error_msg());
			}

			/*
			* @brief 协程挂起，直到ssl完成一次写操作
			*
			* @param data 要写入的数据
			*
			* @return 返回写入的字节数, 0表示对方关闭连接
			*/
			coro::Task<int> write_ssl_once(const CString& data)
			{
				while (true)
				{
					int num_written = net::write(ssl_, data);
					if (num_written < 0)
						co_await ssl_eagain_wait(num_written);
					else
						co_return num_written;
				}
			}

			/*
			* @brief 协程挂起，直到ssl完成一次写操作
			*
			* @param buffer 要写入的数据
			*
			* @return 返回写入的字节数, 0表示对方关闭连接
			*/
			coro::Task<int> write_ssl_once(Buffer& buffer)
			{
				while (true)
				{
					int num_written = net::write(ssl_, buffer);

					if (num_written < 0)
						co_await ssl_eagain_wait(num_written);
					else
					{
						buffer.retrieve(num_written);
						co_return num_written;
					}
				}
			}

			/*
			* @brief 协程挂起，直到fd完成一次写操作
			*
			* @param data 要写入的数据
			*
			* @return 返回写入的字节数, 0表示对方关闭连接
			*/
			coro::Task<int> write_fd_once(const CString& data)
			{
				while (true)
				{
					int num_written = net::write(write_fd_, data);
					if (num_written < 0)
						co_await write_awaiter_;
					else
						co_return num_written;
				}
			}

			coro::Task<int> write_fd_once(Buffer& buffer)
			{
				while (true)
				{
					int num_written = net::write(write_fd_, buffer);
					if (num_written < 0)
						co_await write_awaiter_;
					else
					{
						buffer.retrieve(num_written);
						co_return num_written;
					}
				}
			}
		};


		/*
		* @brief 用于监听窗口的类
		*/
		class Acceptor
		{
			using self = Acceptor;

			int fd_;

			coro::IoContext* io_context_;

			coro::EventAwaiter read_awaiter_;

			SSL_CTX* ssl_ctx_;
		public:
			Acceptor(const CString& ip, int port, coro::IoContext* io_context, bool ipv6 = false, SSL_CTX* ssl_ctx = nullptr) :
				fd_(net::socket(ifelse(ipv6, AF_INET6, AF_INET), SOCK_STREAM, IPPROTO_TCP)),
				read_awaiter_(io_context->wait_for_read(fd_)),
				io_context_(io_context),
				ssl_ctx_(ssl_ctx)
			{
				sockaddr_in addr;
				std::memset(&addr, 0, sizeof(addr));
				addr.sin_family = ifelse(ipv6, AF_INET6, AF_INET);
				addr.sin_port = htons(port);
				if (inet_pton(addr.sin_family, ip.c_str(), &addr.sin_addr) != 1)
					RuntimeError("Invalid host address.");
				if (::bind(fd_, (sockaddr*)&addr, sizeof(addr)) != 0)
					RuntimeError(get_error_msg());

				net::setblocking(fd_, false);
			}

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
				Socket sock(fd, io_context_, ssl_ctx_);
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
				ExTask exit([&res] { freeaddrinfo(res); });

				for (addrinfo* p = res; p; p = p->ai_next)
				{
					int fd = net::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
					if (co_await co_connect(fd, p->ai_addr, p->ai_addrlen, io_context))
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
#if defined(AYR_WIN)
		// 用于初始化Winsock的类
		class _StartSocket
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
#endif // AYR_WIN
	}
}
#endif // AYR_NET_SOCKET_HPP