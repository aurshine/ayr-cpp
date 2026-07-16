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

			Session(SSL_CTX* ssl_ctx) : ssl_ctx_(ssl_ctx) {}

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
			coro::Task<HttpResponse> request(coro::IoContext* io_context, Atring method, const Uri& uri, const Dict<Atring, Atring>& headers = {}, const Atring& data = {})
			{
				HttpRequest req(method, uri, "HTTP/1.1"as, true);
				req.add_header("Accept"as, "*/*"as);
				req.add_header("Accept-Encoding"as, "zstd"as);
				req.add_header("Content-Type"as, "text/plain"as);
				for (auto& [key, value] : headers)
					req.add_header(key, value);

				req.set_body(data);

				bool use_tls = req.uri().scheme() == "https"as;
				if (use_tls && ssl_ctx_ == nullptr)
					ssl_ctx_ = create_ssl_ctx();

				Socket sock = co_await open_connect(
					req.host().encode(),
					req.port().toint().first,
					io_context,
					TlsLayer(ifelse(use_tls, ssl_ctx_, nullptr))
				);

				Buffer req_buffer, resp_buffer;
				req_buffer << req;
				net::IoResult write_result = co_await sock.write(req_buffer);
				if (!write_result.ok())
					RuntimeError(write_result.error);
				
				HttpResponse res;
				ResponseParser res_parser;
				do {
					resp_buffer.adjust_util(8192);
					net::IoResult read_result = co_await sock.read(resp_buffer);
					if (!read_result.ok())
						RuntimeError(read_result.error);
					if (read_result.bytes == 0)
						break;
				} while (!res_parser(res, resp_buffer));

				// HTTP响应已完整解析，此时主动发送TLS close_notify，再由Socket析构
				// 关闭TCP。这样对端可以区分正常结束和被截断的TLS连接。
				net::IoResult shutdown_result = co_await sock.shutdown();
				// close_notify写入底层socket失败意味着关闭通知没有可靠送达，不能
				// 静默当作一次完全成功的HTTPS请求结束。
				if (!shutdown_result.ok())
					RuntimeError(shutdown_result.error);

				co_return res;
			}

			coro::Task<HttpResponse> get(coro::IoContext* io_context, const Uri& uri, const Dict<Atring, Atring>& headers = {}, const Atring& data = {})
			{
				co_return co_await request(io_context, "GET"as, uri, headers, data);
			}

			coro::Task<HttpResponse> post(coro::IoContext* io_context, const Uri& uri, const Dict<Atring, Atring>& headers = {}, const Atring& data = {})
			{
				co_return co_await request(io_context, "POST"as, uri, headers, data);
			}
		};

		coro::Task<HttpResponse> get(coro::IoContext* io_context, const Uri& uri, const Dict<Atring, Atring>& headers = {}, const Atring& data = {})
		{
			co_return co_await Session{}.get(io_context, uri, headers, data);
		}

		coro::Task<HttpResponse> get(coro::IoContext* io_context, const Atring& url, const Dict<Atring, Atring>& headers = {}, const Atring& data = {})
		{
			co_return co_await Session{}.get(io_context, uri(url), headers, data);
		}

		coro::Task<HttpResponse> post(coro::IoContext* io_context, const Uri& uri, const Dict<Atring, Atring>& headers = {}, const Atring& data = {})
		{
			co_return co_await Session{}.post(io_context, uri, headers, data);
		}

		coro::Task<HttpResponse> post(coro::IoContext* io_context, const Atring& url, const Dict<Atring, Atring>& headers = {}, const Atring& data = {})
		{
			co_return co_await Session{}.post(io_context, uri(url), headers, data);
		}
	}
}
#endif // AYR_NET_HTTP_CLIENT_HPP
