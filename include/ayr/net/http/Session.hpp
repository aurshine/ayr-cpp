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

				Socket sock = co_await open_connect(
					req.host().encode(),
					req.port().to_int(),
					io_context,
					ssl_ctx(req.uri().scheme() == "https"as)
				);

				Buffer req_buffer, resp_buffer;
				req_buffer << req;
				co_await sock.write(req_buffer);

				HttpResponse res;
				ResponseParser res_parser;
				do {
					if (co_await sock.read(resp_buffer) == 0)
						break;
				} while (!res_parser(res, resp_buffer));

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
		private:
			/*
			* @brief 是否使用SSL上下文
			*
			* 如果使用，则创建SSL上下文，并加载系统 CA 证书目录
			*
			* 如果已经创建过，则直接返回
			*
			* 如果不使用，则直接返回 nullptr
			*/
			SSL_CTX* ssl_ctx(bool use_ssl)
			{
				if (!use_ssl) return nullptr;
				if (ssl_ctx_ != nullptr) return ssl_ctx_;
				ssl_ctx_ = SSL_CTX_new(TLS_client_method());
				if (ssl_ctx_ == nullptr)
					SSLError("Failed to create SSL context");

				// 建议至少要求 TLS1.2
				SSL_CTX_set_min_proto_version(ssl_ctx_, TLS1_2_VERSION);
				// 不限制最高版本（0 表示自动）
				SSL_CTX_set_max_proto_version(ssl_ctx_, 0);

				// 加载系统 CA 证书目录（必须）
				if (!SSL_CTX_set_default_verify_paths(ssl_ctx_))
					RuntimeError("Failed to load system CA certificates");
#ifdef AYR_WIN
				SSL_CTX_load_verify_locations(ssl_ctx_, "D:\\Download\\vcpkg\\buildtrees\\openssl\\src\\nssl-3.3.2-515f0a0017.clean\\demos\\cms\\cacert.pem", nullptr);
#else
				SSL_CTX_set_verify(ssl_ctx_, SSL_VERIFY_PEER, NULL);
#endif
				return ssl_ctx_;
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