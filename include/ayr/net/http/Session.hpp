#ifndef AYR_NET_HTTP_SESSION_HPP
#define AYR_NET_HTTP_SESSION_HPP

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

			Session(self&& other) noexcept : ssl_ctx_(std::exchange(other.ssl_ctx_, nullptr)) {}

			self& operator=(const self& other) = delete;

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
			* 
			* @param streamed 是否为流式响应体
			*/
			coro::Task<HttpResponse> request(
				coro::IoContext* io_context, 
				CString method, 
				Uri uri, 
				HttpHeaders headers = {}, 
				CString data = {}, 
				bool streamed = false
			)
			{
				// 初始化一个http request
				auto req = init_request(method, uri, std::move(headers), std::move(data));

				// 创建tcp连接
				Socket server = co_await open_connect(
					req.host(),
					req.port(),
					io_context,
					TlsLayer(ssl_ctx(req.uri().scheme() == vstr("https")))
				);

				Buffer req_buffer;
				req_buffer << req;
				// 向服务端发送请求
				net::IoResult write_result = co_await server.write(req_buffer);
				if (!write_result.ok())
					RuntimeError(write_result.error);

				// 构造响应体
				HttpResponse resp(std::move(server), streamed, req.method());
				
				co_await resp.wait();
				co_return resp;
			}

			coro::Task<HttpResponse> get(
				coro::IoContext* io_context, 
				Uri uri, 
				HttpHeaders headers = {}, 
				CString data = {}, 
				bool streamed = false
			)
			{
				return request(io_context, vstr("GET"), std::move(uri), std::move(headers), std::move(data), streamed);
			}

			coro::Task<HttpResponse> post(
				coro::IoContext* io_context, 
				Uri uri, 
				HttpHeaders headers = {}, 
				CString data = {},
				bool streamed = false
			)
			{
				return request(io_context, vstr("POST"), std::move(uri), std::move(headers), std::move(data), streamed);
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

			// 初始化一个http请求，返回请求对象
			HttpRequest init_request(const CString& method, const Uri& uri, HttpHeaders&& headers, CString data) const
			{
				// 构造 http 请求
				HttpRequest req(method, uri, vstr("HTTP/1.1"), std::move(headers), false);

				if (!data.empty() && !req.headers.contains(vstr("Content-Type")))
					req.add_header(vstr("Content-Type"), vstr("text/plain; charset=") + cstr(Codec{}));

				// 设置 body
				req.set_body(data);

				return req;
			}
		};

		def request(
			coro::IoContext* io_context, 
			CString method, 
			Uri uri, 
			HttpHeaders headers = {}, 
			CString data = {},
			bool streamed = false
		) -> coro::Task<HttpResponse>
		{
			Session session;
			// 不能使用return，session会先析构，协程仍然保存session的指针
			co_return co_await session.request(io_context, std::move(method), std::move(uri), std::move(headers), std::move(data), streamed);
		}

		def get(
			coro::IoContext* io_context, 
			Uri uri, 
			HttpHeaders headers = {}, 
			CString data = {}, 
			bool streamed = false
		) -> coro::Task<HttpResponse>
		{
			Session session;
			// 不能使用return，session会先析构，协程仍然保存session的指针
			co_return co_await session.request(io_context, vstr("GET"), std::move(uri), std::move(headers), std::move(data), streamed);
		}

		def post(
			coro::IoContext* io_context, 
			Uri uri, 
			HttpHeaders headers = {}, 
			CString data = {}, 
			bool streamed = false
		) -> coro::Task<HttpResponse>
		{
			Session session;
			// 不能使用return，session会先析构，协程仍然保存session的指针
			co_return co_await session.request(io_context, vstr("POST"), std::move(uri), std::move(headers), std::move(data), streamed);
		}
	}
}
#endif // AYR_NET_HTTP_SESSION_HPP
