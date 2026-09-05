#ifndef AYR_NET_SOCKET_HPP
#define AYR_NET_SOCKET_HPP

#include "TlsLayer.hpp"
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
		*
		* 未传入有效TlsLayer时直接使用原始socket收发数据；传入后由TlsLayer负责
		* TLS握手、加密和解密，Selector层不感知TLS。
		*/
		class Socket
		{
			using self = Socket;

			BaseSocket fd_;

			TlsLayer tls_layer_;

			coro::IoContext* io_context_;
		public:
			Socket(BaseSocket fd, coro::IoContext* io_context, TlsLayer tls_layer = {}) :
				fd_(fd),
				tls_layer_(std::move(tls_layer)),
				io_context_(io_context)
			{
				setblocking(fd_, false);
			}

			Socket(self&& other) noexcept :
				fd_(other.fd_),
				tls_layer_(std::move(other.tls_layer_)),
				io_context_(other.io_context_)
			{
				other.fd_ = -1;
				other.io_context_ = nullptr;
			}

			~Socket()
			{
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
			coro::Task<IoResult> handshake(bool is_server, const CString& host = {})
			{
				// 默认TlsLayer代表明文连接，保持handshake为空操作，方便调用方统一流程。
				if (!tls_layer_.enabled())
					co_return IoResult();

				Buffer encrypted_output;
				while (true)
				{
					// 每轮只推进一次OpenSSL状态机。服务端等待ClientHello；客户端
					// 同时设置host用于SNI和证书身份验证。
					TlsResult tls_result = ifelse(is_server,
						tls_layer_.handshake_server(encrypted_output),
						tls_layer_.handshake_client(encrypted_output, host));

					// 无论TLS步骤返回成功、WANT_*还是错误，都可能产生必须发送的
					// 握手record或alert，因此总是在解释状态前先写完encrypted_output。
					if (encrypted_output.readable_size() > 0)
					{
						IoResult write_result = co_await raw_write(encrypted_output);
						// raw_write保证尽量写完整；错误或零进展时继续循环只会忙等。
						if (!write_result.ok() || write_result.bytes == 0)
							co_return write_result;
					}

					switch (tls_result.state)
					{
					case TlsState::Complete:
						// Finished消息已经验证，后续可以安全收发应用数据。
						co_return IoResult();
					case TlsState::WantWrite:
						// wbio已在上方排空；重新进入循环让OpenSSL继续原步骤。
						continue;
					case TlsState::WantRead:
					{
						// 当前rbio数据不足。read_buffer会先保证至少8KiB可写空间，
						// raw_read返回后下一轮把新密文喂给OpenSSL。
						IoResult read_result = co_await raw_read(tls_layer_.read_buffer());
						if (!read_result.ok())
							co_return read_result;
						// 握手完成前TCP EOF不是有序TLS关闭，必须作为握手失败返回。
						if (read_result.bytes == 0)
							co_return io_result(vstr("TLS peer closed the transport during handshake."));
						break;
					}
					case TlsState::Closed:
						// 握手期间收到close_notify，连接未建立，不能作为成功EOF处理。
						co_return io_result(vstr("TLS peer closed the connection during handshake."));
					case TlsState::Error:
						// 协议或证书验证失败不可重试，保留TlsLayer构造的详细错误。
						co_return io_result(std::move(tls_result.error));
					}
				}
			}

			/*
			* @brief 发送TLS close_notify。
			*
			* 该操作只等待close_notify写入底层socket，不等待对端的关闭响应。
			*/
			coro::Task<IoResult> shutdown()
			{
				// 明文Socket无需TLS关闭握手。
				if (!tls_layer_.enabled())
					co_return IoResult();

				Buffer encrypted_output;
				// TlsLayer采用单向关闭：生成本端close_notify，但不等待对端响应。
				TlsResult tls_result = tls_layer_.shutdown(encrypted_output);

				// ret==0（等待对端close_notify）在单向策略下视为成功；只有明确的
				// TLS错误才向上传播。
				if (tls_result.state == TlsState::Error)
					co_return io_result(std::move(tls_result.error));

				if (encrypted_output.readable_size() > 0)
				{
					// 必须在Socket析构关闭TCP之前把close_notify完整写出。
					IoResult write_result = co_await raw_write(encrypted_output);
					if (!write_result.ok())
						co_return write_result;
				}

				co_return IoResult();
			}

			/*
			* @brief 协程挂起，直到socket写完buffer
			*
			* @param buffer 要写入的数据
			*
			* @return IO事件完成结果
			*/
			coro::Task<IoResult> write(Buffer& buffer)
			{
				if (tls_layer_.enabled())
					return tls_write(buffer); 
				else
					return raw_write(buffer);
			}

			/*
			* @brief 协程挂起，直到读取到数据
			*
			* @param buffer 要读取的数据存放的buffer, 读取数据的字节数取决于buffer的可写空间
			*
			* @return IO事件完成结果
			*/
			coro::Task<IoResult> read(Buffer& buffer)
			{
				if (tls_layer_.enabled())
					return tls_read(buffer);
				else
					return raw_read(buffer);
			}

			constexpr std::strong_ordering operator<=>(const self& other) const { return fd_ <=> other.fd_; }

			constexpr bool operator==(const self& other) const { return fd_ == other.fd_; }

			hash_t __hash__() const { return fd_; }

			void __repr__(Buffer& buffer) const { buffer << "Socket(" << fd_ << ")"; }
		private:
			/*
			* @brief 使用现有IO模型发送buffer中的原始二进制数据
			*
			* 仅供未配置TlsLayer的明文连接使用。
			*/
			coro::Task<IoResult> raw_write(Buffer& buffer)
			{
				IoResult ret;
				while (buffer.readable_size() > 0)
				{
					auto result = co_await io_context_->write_awaiter(fd_, &buffer);
					ret.bytes += result.bytes;
					if (!result.ok() || result.bytes == 0)
					{
						ret.error = std::move(result.error);
						break;
					}
				}
				co_return ret;
			}

			/*
			* @brief 使用现有IO模型读取原始二进制数据
			*
			* 仅供未配置TlsLayer的明文连接使用。
			*/
			coro::Task<IoResult> raw_read(Buffer& buffer)
			{
				co_return co_await io_context_->read_awaiter(fd_, &buffer);
			}

			// 读取数据并由tls层解密
			coro::Task<IoResult> tls_read(Buffer& buffer)
			{
				Buffer encrypted_output;
				while (true)
				{
					// 先尝试消费SSL内部已缓冲的record，再决定是否读取socket。
					// 这可避免SSL已有明文时仍挂起等待新的网络事件。
					TlsResult tls_result = tls_layer_.decrypt(buffer, encrypted_output);

					// SSL_read_ex可能生成密钥更新响应或alert，读取路径也需要发送wbio。
					if (encrypted_output.readable_size() > 0)
					{
						IoResult write_result = co_await raw_write(encrypted_output);
						if (!write_result.ok() || write_result.bytes == 0)
							co_return write_result;
					}

					switch (tls_result.state)
					{
					case TlsState::Complete:
					{
						// 一次read返回当前产生的明文字节；剩余SSL内部数据留给下次
						// Socket::read，保持普通socket“读取一批数据”的语义。
						IoResult result;
						result.bytes = tls_result.bytes;
						co_return result;
					}
					case TlsState::WantWrite:
						// 控制消息已在上方发送，立即重试读取以继续状态机。
						continue;
					case TlsState::WantRead:
					{
						// 当前record可能只收到一部分；读取下一批网络密文后重试。
						IoResult read_result = co_await raw_read(tls_layer_.read_buffer());
						if (!read_result.ok())
							co_return read_result;
						// 未先收到TLS close_notify就出现TCP EOF可能是截断攻击，不能
						// 与正常的Closed状态混为一谈。
						if (read_result.bytes == 0)
							co_return io_result(
								vstr("TLS peer closed the transport without close_notify.")
							);
						break;
					}
					case TlsState::Closed:
						// 对端close_notify映射为成功且bytes==0，与普通socket EOF一致。
						co_return IoResult();
					case TlsState::Error:
						// MAC、record格式、证书后握手消息等错误不可恢复。
						co_return io_result(std::move(tls_result.error));
					}
				}
			}

			// tls层加密并发送数据
			coro::Task<IoResult> tls_write(Buffer& buffer)
			{
				Buffer encrypted_output;
				IoResult ret;
				// SSL_write_ex可能只消费部分明文；循环直到调用方Buffer全部被消费。
				while (buffer.readable_size() > 0)
				{
					TlsResult tls_result = tls_layer_.encrypt(buffer, encrypted_output);
					// 对外报告明文字节数，而不是TLS record和协议开销后的密文字节数。
					ret.bytes += tls_result.bytes;

					// 先发送本轮产生的所有密文。即使TLS结果是Error，其中也可能包含
					// 需要告知对端的fatal alert。
					if (encrypted_output.readable_size() > 0)
					{
						IoResult write_result = co_await raw_write(encrypted_output);
						if (!write_result.ok() || write_result.bytes == 0)
						{
							// 明文可能已被OpenSSL消费，所以同时保留已消费字节数和
							// 底层传输错误，供调用方判断写入进度。
							ret.error = std::move(write_result.error);
							co_return ret;
						}
					}

					switch (tls_result.state)
					{
					case TlsState::Complete:
						// 本轮明文已消费；若buffer仍有数据，外层循环继续加密。
					case TlsState::WantWrite:
						// wbio已排空，保持原buffer内容并重新调用SSL_write_ex。
						break;
					case TlsState::WantRead:
					{
						// TLS 1.3 post-handshake消息可能让写操作依赖对端输入；
						// 读取密文后，外层循环以未消费的相同明文参数重试。
						IoResult read_result = co_await raw_read(tls_layer_.read_buffer());
						if (!read_result.ok())
							co_return read_result;
						// 写入过程中传输层直接关闭且没有close_notify属于异常中断。
						if (read_result.bytes == 0)
							co_return io_result(
								vstr("TLS peer closed the transport while writing.")
							);
						break;
					}
					case TlsState::Closed:
						// 收到对端close_notify后禁止继续写应用数据。
						co_return io_result(
							vstr("Cannot write after the TLS peer sent close_notify."),
							ret.bytes
						);
					case TlsState::Error:
						// 保留此前已成功消费的明文字节数，并附带TLS错误原因。
						ret.error = std::move(tls_result.error);
						co_return ret;
					}
				}
				co_return ret;
			}
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

			TlsLayer tls_layer_;
		public:
			Acceptor(const CString& ip, int port, coro::IoContext* io_context, int family = AF_INET, TlsLayer tls_layer = {}) :
				family_(family),
				fd_(net::socket(family, SOCK_STREAM, IPPROTO_TCP)),
				io_context_(io_context),
				tls_layer_(std::move(tls_layer))
			{
				net::reuse_addr(fd_, true);

				if (family == AF_INET)
				{
					sockaddr_in addr;
					std::memset(&addr, 0, sizeof(addr));
					addr.sin_family = family_;
					addr.sin_port = htons(port);
					int parse_result = inet_pton(addr.sin_family, ip.c_str(), &addr.sin_addr);
					if (parse_result == 0)
						RuntimeError("Invalid host address.");
					if (parse_result < 0)
						RuntimeError(get_socket_error_msg());
					if (::bind(fd_, (sockaddr*)&addr, sizeof(addr)) != 0)
						RuntimeError(get_socket_error_msg());
				}
				else if (family == AF_INET6)
				{
					sockaddr_in6 addr;
					std::memset(&addr, 0, sizeof(addr));
					addr.sin6_family = family_;
					addr.sin6_port = htons(port);
					int parse_result = inet_pton(addr.sin6_family, ip.c_str(), &addr.sin6_addr);
					if (parse_result == 0)
						RuntimeError("Invalid host address.");
					if (parse_result < 0)
						RuntimeError(get_socket_error_msg());
					if (::bind(fd_, (sockaddr*)&addr, sizeof(addr)) != 0)
						RuntimeError(get_socket_error_msg());
				}
				

				net::setblocking(fd_, false);
			}

			Acceptor(const CString& ip, int port, coro::IoContext* io_context, TlsLayer tls_layer, int family = AF_INET) :
				Acceptor(ip, port, io_context, family, std::move(tls_layer)) {}

			Acceptor(self&& other) noexcept :
				family_(other.family_),
				fd_(other.fd_),
				io_context_(other.io_context_),
				tls_layer_(std::move(other.tls_layer_))
			{
				other.fd_ = -1;
				other.io_context_ = nullptr;
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
					RuntimeError(get_socket_error_msg());
			}

			coro::Task<Socket> accept()
			{
				IoResult result = co_await io_context_->accept_awaiter(fd_, family_);
				
				if (!result.ok())
					RuntimeError(result.error);
				
				Socket sock(result.socket, io_context_, tls_layer_.new_session());
				IoResult handshake_result = co_await sock.handshake(true);
				if (!handshake_result.ok())
					RuntimeError(handshake_result.error);
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
		def open_connect(const CString& host, int port, coro::IoContext* io_context, TlsLayer tls_layer = {}) -> coro::Task<Socket>
		{
			addrinfo hints, * res = nullptr;
			CString last_error;
			memset(&hints, 0, sizeof(hints));

			// 尝试IPv4和IPv6
			hints.ai_family = AF_UNSPEC;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = IPPROTO_TCP;

			int resolve_error = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res);
			if (resolve_error == 0)
			{
				exitask([&res] { freeaddrinfo(res); });

				for (addrinfo* p = res; p; p = p->ai_next)
				{
					BaseSocket fd = net::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
					auto result = co_await io_context->connect_awaiter(fd, p);
					if (result.ok())
					{
						Socket sock(fd, io_context, std::move(tls_layer));
						IoResult handshake_result = co_await sock.handshake(false, host);
						if (!handshake_result.ok())
							RuntimeError(handshake_result.error);
						co_return sock;
					}
					last_error = result.error;
					net::close(fd);
				}
			}
			else
			{
				RuntimeError(gai_error2str(resolve_error));
			}

			if (!last_error.empty())
				RuntimeError(ayr::format("Failed to connect to server: {}", last_error));
			RuntimeError("Failed to connect to server.");
			co_return Socket(-1, io_context);
		}
	}
}
#endif // AYR_NET_SOCKET_HPP
