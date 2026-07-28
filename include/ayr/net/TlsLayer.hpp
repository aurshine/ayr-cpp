#ifndef AYR_NET_TLSLAYER_HPP
#define AYR_NET_TLSLAYER_HPP

#include "utils.hpp"
#include "Selector/IoResult.hpp"
#include "../base/raise_error.hpp"

#ifdef AYR_WIN
#include <wincrypt.h>
#endif

namespace ayr
{
	namespace net
	{
		enum class TlsState
		{
			// 本次TLS步骤已完成。握手时表示握手结束；读写时表示产生或消费了应用数据。
			Complete,
			// OpenSSL需要更多对端密文。调用方应先发送wbio中的待发数据，再读取socket。
			WantRead,
			// OpenSSL暂时无法继续写入BIO。调用方应先排空并发送wbio，然后重试原操作。
			WantWrite,
			// 对端发送了close_notify，TLS层已经有序关闭读方向。
			Closed,
			// 协议、证书或BIO发生不可重试错误，详细原因保存在error中。
			Error
		};

		struct TlsResult
		{
			using self = TlsResult;

			TlsState state;
			CString error; // 仅Error状态有意义，必须拥有自己的存储以跨协程安全传递。
			int bytes;     // encrypt表示已消费的明文字节；decrypt表示已产生的明文字节。

			TlsResult(TlsState state = TlsState::Complete, int bytes = 0) :
				state(state), error(), bytes(bytes) {}

			TlsResult(const self& other) :
				state(other.state),
				error(other.error.clone()),
				bytes(other.bytes) {}

			TlsResult(self&& other) noexcept :
				state(other.state),
				error(std::move(other.error)),
				bytes(other.bytes) {}

			self& operator=(const self& other)
			{
				if (this == &other) return *this;
				ayr_destroy(this);
				return *ayr_construct(this, other);
			}

			self& operator=(self&& other) noexcept
			{
				if (this == &other) return *this;
				ayr_destroy(this);
				return *ayr_construct(this, std::move(other));
			}

			bool ok() const { return state != TlsState::Error; }
		};

		SSL_CTX* retain_ssl_ctx(SSL_CTX* ssl_ctx)
		{
			// TlsLayer和Session可共享SSL_CTX，但各自必须持有一个引用；这样调用方
			// 构造TlsLayer后立即释放自己的引用也不会破坏现有会话。
			if (ssl_ctx != nullptr && SSL_CTX_up_ref(ssl_ctx) != 1)
				SSLError("Failed to retain SSL context.");
			return ssl_ctx;
		}

		/*
		* @brief 基于OpenSSL Memory BIO的TLS协议层
		*
		* TlsLayer只负责TLS握手、加密、解密和协议缓冲，不执行任何socket IO。
		* Socket直接将网络密文读入TlsLayer的输入buffer，并发送每次TLS操作返回的密文。
		*
		* TlsLayer通过OpenSSL引用计数共享SSL_CTX，调用方可以在构造TlsLayer后释放
		* 自己持有的引用。每个TlsLayer拥有独立的SSL会话，不能在连接之间共享。
		*/
		class TlsLayer
		{
			using self = TlsLayer;

			// Socket直接写入该buffer；尚未处理的数据可与下一次读取的数据合并。
			Buffer encrypted_input_;

			// 通过SSL_CTX_up_ref持有的共享引用，在析构时通过SSL_CTX_free释放。
			SSL_CTX* ssl_ctx_;

			// 当前连接独占的TLS会话；内部BIO会继续保存OpenSSL已接收的不完整记录。
			SSL* ssl_;

			enum class Mode
			{
				Unset,
				Client,
				Server
			};

			Mode mode_;
		public:
			TlsLayer() :
				encrypted_input_(),
				ssl_ctx_(nullptr),
				ssl_(nullptr),
				mode_(Mode::Unset) {}

			TlsLayer(SSL_CTX* ssl_ctx) :
				encrypted_input_(),
				ssl_ctx_(retain_ssl_ctx(ssl_ctx)),
				ssl_(nullptr),
				mode_(Mode::Unset) {}

			TlsLayer(const self& other) = delete;

			TlsLayer(self&& other) noexcept :
				encrypted_input_(std::move(other.encrypted_input_)),
				ssl_ctx_(std::exchange(other.ssl_ctx_, nullptr)),
				ssl_(std::exchange(other.ssl_, nullptr)),
				mode_(std::exchange(other.mode_, Mode::Unset)) {}

			~TlsLayer()
			{
				if (ssl_)
					SSL_free(ssl_);
				if (ssl_ctx_)
					SSL_CTX_free(ssl_ctx_);
			}

			self& operator=(self&& other) noexcept
			{
				if (this == &other) return *this;
				ayr_destroy(this);
				return *ayr_construct(this, std::move(other));
			}

			// 未传入SSL_CTX的默认TlsLayer表示不启用TLS。
			bool enabled() const { return ssl_ctx_ != nullptr; }

			/*
			* @brief 创建共享SSL_CTX、但拥有独立SSL会话的新TLS层
			*/
			self new_session() const { return self(ssl_ctx_); }

			/*
			* @brief 获取接收网络密文的buffer
			*
			* Socket的raw_read直接写入该buffer，避免经过临时buffer再次复制。
			*/
			Buffer& read_buffer()
			{
				// raw_read会直接写入该Buffer。每次交给socket前保证至少还有8KiB连续
				// 可写空间，既避免64字节默认容量造成TLS record过度分片，也保留尚未
				// 喂入rbio的尾部数据。
				encrypted_input_.adjust_util(8192);
				return encrypted_input_;
			}

			TlsResult handshake_client(Buffer& encrypted_output, const CString& host = {})
			{
				// 未配置上下文表示该Socket走明文路径；这里保持空操作语义。
				if (!enabled()) return TlsResult();
				// 客户端角色、SNI和证书校验目标只能在首次握手前设置。
				initialize_client(host);
				// handshake只推进一步，网络读写由Socket根据返回状态驱动。
				return handshake(encrypted_output);
			}

			TlsResult handshake_server(Buffer& encrypted_output)
			{
				// Acceptor未配置TLS时，新连接无需执行任何握手步骤。
				if (!enabled()) return TlsResult();
				// 服务端证书和私钥由调用方传入的SSL_CTX提供。
				initialize_server();
				return handshake(encrypted_output);
			}

			/*
			* @brief tls加密数据
			* 
			* @param data 需要加密的原始数据
			* 
			* @param output 加密后的数据保存buffer
			*/
			TlsResult encrypt(Buffer& data, Buffer& encrypted_output)
			{
				initialize();
				// 应用数据不能先于握手发送，否则SSL_write_ex会隐式推进握手，
				// 使Socket无法判断输出究竟是握手消息还是应用数据。
				if (!SSL_is_init_finished(ssl_))
					return tls_error("Cannot encrypt application data before the TLS handshake completes.");

				// 写操作也可能需要消费TLS 1.3会话票据、密钥更新等对端消息，
				// 因而先把Socket已读取的密文全部交给rbio。
				TlsResult feed_result = feed_encrypted_input();
				if (feed_result.state != TlsState::Complete)
					return feed_result;

				// 没有新明文时仍排空wbio，避免此前生成的alert或控制消息滞留。
				if (data.readable_size() == 0)
					return drain_encrypted_output(encrypted_output);

				size_t num_written = 0;
				// SSL_get_error依赖当前线程错误队列只包含本次调用的信息，因此每次
				// SSL_*调用前清空旧错误，并在任何其他OpenSSL调用前立即取错误码。
				ERR_clear_error();
				int ret = SSL_write_ex(
					ssl_,
					data.peek(),
					static_cast<size_t>(data.readable_size()),
					&num_written
				);
				int ssl_error = ret == 1 ? SSL_ERROR_NONE : SSL_get_error(ssl_, ret);
				TlsResult result = classify_ssl_result("Failed to write SSL data", ret, ssl_error);
				if (ret == 1)
				{
					// 只有SSL_write_ex明确成功后才能消费输入。WANT_*要求使用相同
					// 参数重试，所以失败/等待分支必须保留原始data不动。
					data.retrieve(static_cast<c_size>(num_written));
					result.bytes = static_cast<int>(num_written);
				}

				// SSL_write_ex即使返回WANT_READ或失败，也可能已经生成握手消息或
				// alert；所有分支都必须排空wbio并交给Socket发送。
				TlsResult drain_result = drain_encrypted_output(encrypted_output);
				if (
					(drain_result.state == TlsState::Error && result.state != TlsState::Error)
					|| (drain_result.state == TlsState::WantWrite && result.state == TlsState::Complete)
				)
					// 原操作成功时，BIO排空失败就是当前最重要的状态；原操作已经
					// 失败时则保留更接近根因的SSL错误。
					return drain_result;
				return result;
			}

			/*
			* @brief tls解密数据
			* 
			* @param output 解密后的数据保存buffer
			*/
			TlsResult decrypt(Buffer& output, Buffer& encrypted_output)
			{
				initialize();
				// Socket::handshake保证正常调用路径不会触发此分支；保留检查可防止
				// 直接使用TlsLayer时把握手密文误当成应用明文。
				if (!SSL_is_init_finished(ssl_))
					return tls_error("Cannot decrypt application data before the TLS handshake completes.");

				// encrypted_input_可能只包含TLS record的一部分。Memory BIO会保存
				// 这些字节，后续SSL_read_ex通过WantRead请求下一批网络数据。
				TlsResult feed_result = feed_encrypted_input();
				if (feed_result.state != TlsState::Complete)
					return feed_result;

				// 每次最多向调用方当前Buffer追加数据；至少预留8KiB，后续若SSL内部
				// 仍有明文，下一次Socket::read会先再次调用decrypt而不是等待网络。
				output.adjust_util(8192);
				size_t num_read = 0;
				ERR_clear_error();
				int ret = SSL_read_ex(
					ssl_,
					output.write_ptr(),
					static_cast<size_t>(output.writeable_size()),
					&num_read
				);
				int ssl_error = ret == 1 ? SSL_ERROR_NONE : SSL_get_error(ssl_, ret);
				TlsResult result = classify_ssl_result("Failed to read SSL data", ret, ssl_error);
				if (ret == 1)
				{
					// OpenSSL只写入了num_read字节，必须同步推进Buffer写指针。
					output.written(static_cast<c_size>(num_read));
					result.bytes = static_cast<int>(num_read);
				}

				// SSL_read_ex可能响应密钥更新、post-handshake认证或生成alert，
				// 因此读取路径同样必须把wbio输出交给Socket。
				TlsResult drain_result = drain_encrypted_output(encrypted_output);
				if (
					(drain_result.state == TlsState::Error && result.state != TlsState::Error)
					|| (drain_result.state == TlsState::WantWrite && result.state == TlsState::Complete)
				)
					// 仅在原读取没有更具体错误时，用BIO排空状态覆盖结果。
					return drain_result;
				return result;
			}

			/*
			* @brief 发送本端close_notify。
			*
			* Memory BIO模式下只完成单向关闭；调用方发送encrypted_output后即可关闭传输层，
			* 不会为了等待对端close_notify而无限阻塞。
			*/
			TlsResult shutdown(Buffer& encrypted_output)
			{
				// 明文连接或从未创建SSL会话时没有close_notify可发送。
				if (!enabled() || ssl_ == nullptr)
					return TlsResult();

				// 先处理已经收到的close_notify。若对端已经关闭，SSL_shutdown可能
				// 一次完成双向关闭；否则本函数只生成本端close_notify。
				TlsResult feed_result = feed_encrypted_input();
				if (feed_result.state != TlsState::Complete)
					return feed_result;

				ERR_clear_error();
				int ret = SSL_shutdown(ssl_);
				int ssl_error = ret >= 0 ? SSL_ERROR_NONE : SSL_get_error(ssl_, ret);
				TlsResult result;
				// ret==1表示双向关闭完成；ret==0表示本端close_notify已生成、仍可
				// 等待对端响应。这里采用单向关闭策略，二者都视为本步骤完成。
				if (ret < 0)
					result = classify_ssl_result("Failed to shut down TLS connection", ret, ssl_error);

				// close_notify本身位于wbio，必须在底层TCP关闭前发送出去。
				TlsResult drain_result = drain_encrypted_output(encrypted_output);
				if (
					(drain_result.state == TlsState::Error && result.state != TlsState::Error)
					|| (drain_result.state == TlsState::WantWrite && result.state == TlsState::Complete)
				)
					return drain_result;
				return result;
			}
		private:
			static TlsResult tls_error(CString error)
			{
				TlsResult result(TlsState::Error);
				// CString的普通复制只是非拥有视图。错误会跨越当前调用甚至协程挂起，
				// 所以拥有型字符串直接移动，视图则显式clone，避免悬空。
				if (error.owner())
					result.error = std::move(error);
				else
					result.error = error.clone();
				return result;
			}

			static bool is_ip_address(const CString& host)
			{
				in_addr ipv4;
				in6_addr ipv6;
				return ::inet_pton(AF_INET, host.c_str(), &ipv4) == 1
					|| ::inet_pton(AF_INET6, host.c_str(), &ipv6) == 1;
			}

			void initialize()
			{
				// SSL和BIO按需创建。握手、读、写可能多次进入这里，但一个连接只能
				// 拥有一套状态机，不能重复创建。
				if (ssl_ != nullptr)
					return;
				// enabled()==false只允许走Socket明文分支；直接调用TLS操作属于误用。
				if (!enabled())
					RuntimeError("Cannot initialize TLS layer without SSL context.");

				ssl_ = SSL_new(ssl_ctx_);
				if (!ssl_)
					SSLError("Failed to create SSL object.");

				// rbio接收网络密文，wbio保存OpenSSL生成、等待Socket发送的密文。
				// 两个BIO必须独立，否则双向数据和所有权都会混淆。
				BIO* read_bio = BIO_new(BIO_s_mem());
				BIO* write_bio = BIO_new(BIO_s_mem());
				if (!read_bio || !write_bio)
				{
					// SSL_set_bio尚未接管BIO所有权，失败路径必须逐一释放；随后释放
					// 半初始化SSL并复位指针，保证析构不会二次释放。
					BIO_free(read_bio);
					BIO_free(write_bio);
					SSL_free(ssl_);
					ssl_ = nullptr;
					SSLError("Failed to create TLS memory BIO.");
				}

				// 空Memory BIO返回-1并设置retry语义，使OpenSSL产生WANT_READ/WRITE，
				// 而不是把“暂时无数据”误判为传输层EOF。
				BIO_set_mem_eof_return(read_bio, -1);
				BIO_set_mem_eof_return(write_bio, -1);
				// 成功后SSL取得两个BIO的所有权，最终由SSL_free统一释放。
				SSL_set_bio(ssl_, read_bio, write_bio);
			}

			void initialize_client(const CString& host)
			{
				initialize();
				// 同一个SSL对象的角色一旦确定就不能切换，否则会破坏握手状态。
				if (mode_ == Mode::Server)
					RuntimeError("Cannot use a server TLS session as a client.");
				// 握手会多次调用本函数；客户端参数只能在第一次调用时配置。
				if (mode_ == Mode::Client)
					return;

				SSL_set_connect_state(ssl_);
				if (!host.empty())
				{
					// CString不保证尾部有NUL；StringZero在本作用域内提供稳定的
					// NUL结尾字符串，供OpenSSL C API安全使用。
					StringZero hostname = host.c_str();
					const char* hostname_ptr = hostname.c_str();
					ERR_clear_error();
					if (is_ip_address(host))
					{
						// IP证书匹配必须检查subjectAltName中的iPAddress，不能使用
						// DNS主机名规则，也不发送没有意义的SNI IP字面量。
						X509_VERIFY_PARAM* verify_param = SSL_get0_param(ssl_);
						if (X509_VERIFY_PARAM_set1_ip_asc(verify_param, hostname_ptr) != 1)
							SSLError(ayr::format(
								"Failed to set TLS certificate IP address: {}",
								ssl_error_msg()
							));
					}
					else
					{
						// SNI用于让同一IP上的虚拟主机选择正确证书；它本身不做
						// 安全校验，所以还必须单独调用SSL_set1_host。
						if (SSL_set_tlsext_host_name(ssl_, hostname_ptr) != 1)
							SSLError(ayr::format(
								"Failed to set TLS SNI hostname: {}",
								ssl_error_msg()
							));

						ERR_clear_error();
						// 把期望DNS名写入验证参数，握手验证证书链时自动检查SAN/CN，
						// 防止“证书受信任但签发给其他域名”的中间人攻击。
						if (SSL_set1_host(ssl_, hostname_ptr) != 1)
							SSLError(ayr::format(
								"Failed to set TLS certificate hostname: {}",
								ssl_error_msg()
							));
					}
				}
				// 所有握手前参数成功设置后才提交角色，避免配置异常留下半完成状态。
				mode_ = Mode::Client;
			}

			void initialize_server()
			{
				initialize();
				// 与客户端路径对称，禁止在已有客户端状态的SSL上切换角色。
				if (mode_ == Mode::Client)
					RuntimeError("Cannot use a client TLS session as a server.");
				// 后续握手step无需重复设置accept状态。
				if (mode_ == Mode::Server)
					return;

				// 证书、私钥及客户端证书策略均来自构造时传入的服务端SSL_CTX。
				SSL_set_accept_state(ssl_);
				mode_ = Mode::Server;
			}

			TlsResult classify_ssl_result(const char* operation, int ret, int ssl_error)
			{
				// SSL_*_ex和SSL_do_handshake统一以1表示当前操作完成。
				if (ret == 1)
					return TlsResult();

				switch (ssl_error)
				{
				case SSL_ERROR_WANT_READ:
					// 正常的非阻塞状态：rbio数据不足。Socket发送已有wbio数据后，
					// 从网络读取更多密文，再以原参数重试。
					return TlsResult(TlsState::WantRead);
				case SSL_ERROR_WANT_WRITE:
					// 正常的反压状态：先排空/发送wbio，再重试当前SSL操作。
					return TlsResult(TlsState::WantWrite);
				case SSL_ERROR_ZERO_RETURN:
					// 对端已发送close_notify。这不是传输错误，调用方应把读结果
					// 解释为有序EOF。
					return TlsResult(TlsState::Closed);
				default:
				{
					// 其余状态不可自动重试。立即消费OpenSSL错误队列，防止后续
					// OpenSSL调用覆盖真正原因。
					CString error = (
						vstr(operation)
						+ vstr(": ")
						+ ssl_error_msg(ssl_error, ret)
					);
					long verify_result = SSL_get_verify_result(ssl_);
					if (verify_result != X509_V_OK)
					{
						// 握手因证书失败时，错误队列通常只有通用SSL错误；追加
						// X509验证原因可明确区分过期、未知CA和主机名不匹配。
						error += (
							vstr("; certificate verification failed: ")
							+ vstr(X509_verify_cert_error_string(verify_result))
						);
					}
					return tls_error(std::move(error));
				}
				}
			}

			TlsResult feed_encrypted_input()
			{
				BIO* rbio = SSL_get_rbio(ssl_);
				// BIO_write_ex可能只消费部分输入，因此持续写到encrypted_input_为空；
				// 每次成功后只retrieve实际消费的字节。
				while (encrypted_input_.readable_size() > 0)
				{
					size_t bytes_written = 0;
					ERR_clear_error();
					int ret = BIO_write_ex(
						rbio,
						encrypted_input_.peek(),
						static_cast<size_t>(encrypted_input_.readable_size()),
						&bytes_written
					);
					if (ret != 1)
					{
						// Memory BIO通常不会产生反压，但仍保留retry分支以兼容未来
						// 替换为有容量限制的BIO。输入未retrieve，可安全重试。
						if (BIO_should_retry(rbio))
							return TlsResult(TlsState::WantRead);
						// 非retry错误意味着密文无法交给OpenSSL，继续握手/解密没有意义。
						return tls_error(
							vstr("Failed to write encrypted TLS data: ")
							+ ssl_error_msg()
						);
					}
					// 成功却没有任何进展会让外层while永久循环，显式报错防止忙等。
					if (bytes_written == 0)
						return tls_error("TLS read BIO accepted no encrypted data.");
					encrypted_input_.retrieve(static_cast<c_size>(bytes_written));
				}
				return TlsResult();
			}

			TlsResult drain_encrypted_output(Buffer& encrypted_output)
			{
				BIO* wbio = SSL_get_wbio(ssl_);
				// 一次SSL操作可能生成多条TLS record。循环读取，直到wbio完全为空，
				// 保证Socket不会遗漏握手片段、密钥更新或alert。
				while (true)
				{
					size_t pending = BIO_ctrl_pending(wbio);
					// 没有待发密文即完成本次排空。
					if (pending == 0)
						return TlsResult();

					// 先按精确pending大小扩容，随后BIO直接写入Buffer尾部，避免中间复制。
					encrypted_output.adjust_util(static_cast<c_size>(pending));
					size_t bytes_read = 0;
					ERR_clear_error();
					int ret = BIO_read_ex(
						wbio,
						encrypted_output.write_ptr(),
						pending,
						&bytes_read
					);
					if (ret != 1)
					{
						// retry表示wbio暂不可读，调用方稍后重新排空；其他错误说明
						// 无法取得必须发送的TLS数据。
						if (BIO_should_retry(wbio))
							return TlsResult(TlsState::WantWrite);
						return tls_error(
							vstr("Failed to read encrypted TLS data: ")
							+ ssl_error_msg()
						);
					}
					// pending>0却读取0字节表示BIO没有进展，返回错误避免无限循环。
					if (bytes_read == 0)
						return tls_error("TLS write BIO returned no encrypted data.");
					encrypted_output.written(static_cast<c_size>(bytes_read));
				}
			}

			TlsResult handshake(Buffer& encrypted_output)
			{
				// 服务端需要客户端Hello、客户端需要服务端Hello；每一步都先把最近
				// raw_read得到的密文写入rbio，再让OpenSSL推进状态机。
				TlsResult feed_result = feed_encrypted_input();
				if (feed_result.state != TlsState::Complete)
					return feed_result;

				ERR_clear_error();
				int ret = SSL_do_handshake(ssl_);
				// SSL_get_error必须紧跟失败调用，不能先执行BIO操作。
				int ssl_error = ret == 1 ? SSL_ERROR_NONE : SSL_get_error(ssl_, ret);
				TlsResult result = classify_ssl_result(
					"Failed to complete TLS handshake",
					ret,
					ssl_error
				);

				// ClientHello、ServerHello、证书、Finished和alert都从wbio输出；
				// 即使当前状态是WantRead，也要先把已经生成的数据发送给对端。
				TlsResult drain_result = drain_encrypted_output(encrypted_output);
				if (
					(drain_result.state == TlsState::Error && result.state != TlsState::Error)
					|| (drain_result.state == TlsState::WantWrite && result.state == TlsState::Complete)
				)
					// SSL步骤成功但无法取出待发数据时，以BIO状态为准；若SSL本身已
					// 报错则保留原始握手错误，避免掩盖根因。
					return drain_result;
				return result;
			}
		};

#ifdef AYR_WIN
		inline void load_windows_root_certificates(SSL_CTX* ssl_ctx)
		{
			// OpenSSL在Windows上不会自动使用系统“受信任的根证书颁发机构”。
			// 打开当前Windows逻辑ROOT存储，并把其中证书复制到SSL_CTX的X509_STORE。
			HCERTSTORE windows_store = CertOpenSystemStoreW(0, L"ROOT");
			if (windows_store == nullptr)
				SSLError(ayr::format(
					"Failed to open Windows root certificate store: {}",
					win_error2str(GetLastError())
				));

			X509_STORE* openssl_store = SSL_CTX_get_cert_store(ssl_ctx);
			PCCERT_CONTEXT certificate_context = nullptr;
			int loaded_certificates = 0;

			// CertEnumCertificatesInStore会在下一次枚举时释放上一个context，因此
			// 不单独调用CertFreeCertificateContext。
			while (
				(certificate_context = CertEnumCertificatesInStore(
					windows_store,
					certificate_context
				)) != nullptr
			)
			{
				// Windows存储提供DER编码；d2i_X509创建OpenSSL可识别的X509对象。
				const unsigned char* encoded_certificate = certificate_context->pbCertEncoded;
				X509* certificate = d2i_X509(
					nullptr,
					&encoded_certificate,
					static_cast<long>(certificate_context->cbCertEncoded)
				);
				if (certificate == nullptr)
				{
					// 个别系统证书可能使用当前OpenSSL不支持的编码。跳过该证书，
					// 同时清理解析错误，避免污染后续TLS操作的错误队列。
					ERR_clear_error();
					continue;
				}

				// X509_STORE_add_cert会增加内部引用，局部X509随后可以安全释放。
				// 重复证书是正常情况，不应使整个根存储加载失败。
				if (X509_STORE_add_cert(openssl_store, certificate) == 1)
					++loaded_certificates;
				else
					ERR_clear_error();
				X509_free(certificate);
			}

			// 枚举结束后立即关闭Windows句柄；SSL_CTX只保留转换后的OpenSSL证书。
			CertCloseStore(windows_store, 0);
			// 一个根证书都未成功加载时继续握手会让所有公网证书验证失败，
			// 因而在创建上下文阶段直接报告配置错误。
			if (loaded_certificates == 0)
				SSLError("Windows root certificate store did not contain a usable certificate.");
		}
#endif

		/*
		* @brief 创建并配置一个新的TLS客户端上下文
		*
		* 默认启用对端证书验证，Windows加载系统根证书存储，其他平台加载
		* OpenSSL默认CA路径，并要求TLS 1.2及以上版本。
		* 返回的SSL_CTX拥有一个独立引用，调用方负责通过SSL_CTX_free释放。
		*/
		def create_ssl_ctx() -> SSL_CTX*
		{
			ERR_clear_error();
			// unique_ptr保证后续任一步骤抛异常时都释放半配置的SSL_CTX。
			std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)> ssl_ctx(
				SSL_CTX_new(TLS_client_method()),
				&SSL_CTX_free
			);
			if (!ssl_ctx)
				SSLError(ayr::format("Failed to create SSL context: {}", ssl_error_msg()));

			// 禁止TLS 1.0/1.1；TLS_client_method仍允许OpenSSL协商TLS 1.2或1.3。
			ERR_clear_error();
			if (SSL_CTX_set_min_proto_version(ssl_ctx.get(), TLS1_2_VERSION) != 1)
				SSLError(ayr::format("Failed to set minimum TLS version: {}", ssl_error_msg()));

			// 强制验证对端证书链。具体DNS/IP目标在每个SSL会话创建时设置，
			// 因为同一个SSL_CTX可能连接多个不同主机。
			SSL_CTX_set_verify(ssl_ctx.get(), SSL_VERIFY_PEER, nullptr);

#ifdef AYR_WIN
			// Windows使用系统证书管理器中的受信根。
			load_windows_root_certificates(ssl_ctx.get());
#else
			// Unix类平台遵循OpenSSL编译时默认CA文件和目录。
			ERR_clear_error();
			if (SSL_CTX_set_default_verify_paths(ssl_ctx.get()) != 1)
				SSLError(ayr::format("Failed to load default CA certificates: {}", ssl_error_msg()));
#endif

			// 所有配置成功后把独立引用交给调用方；调用方最终用SSL_CTX_free释放。
			return ssl_ctx.release();
		}
	}
}

#endif // AYR_NET_TLSLAYER_HPP
