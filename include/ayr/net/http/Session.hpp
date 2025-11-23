#ifndef AYR_NET_HTTP_CLIENT_HPP
#define AYR_NET_HTTP_CLIENT_HPP

#include "Request.hpp"
#include "Response.hpp"

namespace ayr
{
	namespace net
	{
		class Session : public Object<Session>
		{
			using self = Session;

			using super = Object<Session>;

		public:
			Session() {}

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

				Socket sock = co_await open_connect(req.host().encode(), req.port().to_int(), io_context);

				Buffer req_buffer, resp_buffer;
				req_buffer << req;
				co_await sock.write(req_buffer);

				HttpResponse res;
				ResponseParser res_parser;
				do {
					co_await sock.read(resp_buffer);
				} while (!res_parser(res, resp_buffer));

				co_return res;
			}

			coro::Task<HttpResponse> get(coro::IoContext* io_context, const Uri& uri, const Dict<Atring, Atring>& headers = {}, const Atring& data = {})
			{
				return request(io_context, "GET"as, uri, headers, data);
			}

			coro::Task<HttpResponse> post(coro::IoContext* io_context, const Uri& uri, const Dict<Atring, Atring>& headers = {}, const Atring& data = {})
			{
				return request(io_context, "POST"as, uri, headers, data);
			}
		};

		coro::Task<HttpResponse> get(coro::IoContext* io_context, const Uri& uri, const Dict<Atring, Atring>& headers = {}, const Atring& data = {})
		{
			return Session{}.get(io_context, uri, headers, data);
		}

		coro::Task<HttpResponse> get(coro::IoContext* io_context, const Atring& url, const Dict<Atring, Atring>& headers = {}, const Atring& data = {})
		{
			return Session{}.get(io_context, uri(url), headers, data);
		}

		coro::Task<HttpResponse> post(coro::IoContext* io_context, const Uri& uri, const Dict<Atring, Atring>& headers = {}, const Atring& data = {})
		{
			return Session{}.post(io_context, uri, headers, data);
		}

		coro::Task<HttpResponse> post(coro::IoContext* io_context, const Atring& url, const Dict<Atring, Atring>& headers = {}, const Atring& data = {})
		{
			return Session{}.post(io_context, uri(url), headers, data);
		}
	}
}
#endif // AYR_NET_HTTP_CLIENT_HPP