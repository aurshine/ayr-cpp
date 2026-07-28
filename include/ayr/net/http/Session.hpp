#ifndef AYR_NET_HTTP_CLIENT_HPP
#define AYR_NET_HTTP_CLIENT_HPP

#include "Request.hpp"
#include "Response.hpp"

namespace ayr
{
	namespace net
	{
		class Session
		{
			using self = Session;

			SSL_CTX* ssl_ctx_;
		public:
			Session() : ssl_ctx_(nullptr) {}

			Session(SSL_CTX* ssl_ctx) : ssl_ctx_(retain_ssl_ctx(ssl_ctx)) {}

			Session(const self& other) = delete;

			self& operator=(const self& other) = delete;

			Session(self&& other) noexcept : ssl_ctx_(std::exchange(other.ssl_ctx_, nullptr)) {}

			self& operator=(self&& other) noexcept
			{
				if (this == &other) return *this;
				ayr_destroy(this);
				return *ayr_construct(this, std::move(other));
			}

			~Session()
			{
				if (ssl_ctx_ != nullptr)
					SSL_CTX_free(ssl_ctx_);
			}

			/*
			* @brief 发送http请求
			*
			* @param io_context 协程上下文
			*
			* @param method 请求方法
			*
			* @param uri 请求uri
			*
			* @param headers 请求头
			*
			* @param data 请求体
			*/
			coro::Task<HttpResponse> request(coro::IoContext* io_context, Atring method, Uri uri, HttpHeaders headers = {}, Atring data = {})
			{
				// 构造 http 请求
				HttpRequest req(method, uri, "HTTP/1.1"as, std::move(headers), false);

				if (!data.empty() && !req.headers.contains("Content-Type"as))
					req.add_header("Content-Type"as, "text/plain; charset="as + Atring::from(cstr(Codec{})));
				
				// 设置 body
				req.set_body(data.encode());

				// 验证
				if (req.host().empty())
					ValueError("HTTP requests require an absolute URI with a host.");
				
				auto [port, port_remain] = req.port().toint();
				if (!port_remain.empty() || port <= 0 || port > 65535)
					ValueError(vstr("Invalid HTTP port: ") + cstr(req.port()));

				// 创建tcp连接
				Socket sock = co_await open_connect(
					req.host().encode(),
					port,
					io_context,
					TlsLayer(ssl_ctx(req.uri().scheme() == "https"as))
				);

				Buffer req_buffer, resp_buffer;
				req_buffer << req;
				net::IoResult write_result = co_await sock.write(req_buffer);
				if (!write_result.ok())
					RuntimeError(write_result.error);

				HttpResponse res;
				ResponseParser res_parser(method);
				while (!res_parser(res, resp_buffer))
				{
					resp_buffer.adjust_util(8192);
					net::IoResult read_result = co_await sock.read(resp_buffer);
					if (!read_result.ok())
						RuntimeError(read_result.error);
					if (read_result.bytes == 0)
					{
						res_parser(res, resp_buffer, true);
						break;
					}
				}

				// HTTP响应已完整解析，此时主动发送TLS close_notify，再由Socket析构
				// 关闭TCP。这样对端可以区分正常结束和被截断的TLS连接。
				net::IoResult shutdown_result = co_await sock.shutdown();
				// close_notify写入底层socket失败意味着关闭通知没有可靠送达，不能
				// 静默当作一次完全成功的HTTPS请求结束。
				if (!shutdown_result.ok())
					RuntimeError(shutdown_result.error);

				co_return res;
			}

			coro::Task<HttpResponse> get(coro::IoContext* io_context, Uri uri, HttpHeaders headers = {}, Atring data = {})
			{
				return request(io_context, "GET"as, std::move(uri), std::move(headers), std::move(data));
			}

			coro::Task<HttpResponse> post(coro::IoContext* io_context, Uri uri, HttpHeaders headers = {}, Atring data = {})
			{
				return request(io_context, "POST"as, std::move(uri), std::move(headers), std::move(data));
			}
		private:
			/*
			* @brief 获取session ssl_ctx
			* 
			* @param enable 是否启用ssl
			* 
			* @return 启用ssl后返回ssl_ctx_，否则返回nullptr
			*/ 
			SSL_CTX* ssl_ctx(bool enable)
			{
				if (enable)
				{
					if (ssl_ctx_ == nullptr)
						ssl_ctx_ = create_ssl_ctx();
					return ssl_ctx_;
				}
				return nullptr;
			}
		};

		def request(coro::IoContext* io_context, Atring method, Uri uri, HttpHeaders headers = {}, Atring data = {}) -> coro::Task<HttpResponse>
		{
			Session session;
			return session.request(io_context, std::move(method), std::move(uri), std::move(headers), std::move(data));
		}

		def get(coro::IoContext* io_context, Uri uri, HttpHeaders headers = {}, Atring data = {}) -> coro::Task<HttpResponse>
		{
			Session session;
			return session.request(io_context, "GET"as, std::move(uri), std::move(headers), std::move(data));
		}

		def post(coro::IoContext* io_context, Uri uri, HttpHeaders headers = {}, Atring data = {}) -> coro::Task<HttpResponse>
		{
			Session session;
			return session.request(io_context, "POST"as, std::move(uri), std::move(headers), std::move(data));
		}
	}
}
#endif // AYR_NET_HTTP_CLIENT_HPP
