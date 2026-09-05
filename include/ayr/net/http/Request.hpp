#ifndef AYR_NET_HTTP_REQUEST_HPP
#define AYR_NET_HTTP_REQUEST_HPP

#include "Headers.hpp"
#include "Uri.hpp"
#include "../Socket.hpp"

namespace ayr
{
	namespace net
	{
		class HttpRequest
		{
			using self = HttpRequest;

			// 请求方法
			CString method_;

			// 请求Uri
			Uri uri_;

			// HTTP版本
			CString version_;
		public:
			HttpHeaders headers;

			// 已编码的请求体字节。
			CString body;

			HttpRequest() : method_(), uri_(), version_(), headers(), body() {}

			HttpRequest(CString method, Uri uri, CString version, HttpHeaders headers = {}, bool keep_alive = true) :
				method_(std::move(method)),
				uri_(std::move(uri)),
				version_(std::move(version)),
				headers(std::move(headers))
			{
				// 检查主机名是否有效
				if (uri_.host().empty())
					ValueError("HTTP requests require an absolute URI with a host.");

				// 检查端口号是否有效
				if (!uri_.valid_port())
					ValueError("HTTP requests require an absolute URI with a valid port.");

				// 请求头默认参数
				if (!this->headers.contains(vstr("Host")) && !uri_.host().empty())
					add_header(vstr("Host"), uri_.host());
				if (!this->headers.contains(vstr("Accept")))
					add_header(vstr("Accept"), vstr("*/*"));

				this->keep_alive(keep_alive);
			}

			HttpRequest(const self& other) :
				method_(other.method_),
				uri_(other.uri_),
				version_(other.version_),
				headers(other.headers),
				body(other.body) {
			}

			HttpRequest(self&& other) noexcept :
				method_(std::move(other.method_)),
				uri_(std::move(other.uri_)),
				version_(std::move(other.version_)),
				headers(std::move(other.headers)),
				body(std::move(other.body)) {
			}

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

			// 请求的uri
			const Uri& uri() const { return uri_; }

			// 请求路径 uri.path()?uri.query()
			CString path() const
			{
				if (uri_.queries().empty())
					return uri_.path();
				return vstr("?").join(arr(uri_.path(), uri_.query()));
			}

			// 请求主机名
			const CString& host() const { return uri_.host(); }

			// 请求的端口
			int port() const { return uri_.port(); }

			// 请求的方法
			const CString& method() const { return method_; }

			// 请求的HTTP版本
			const CString& version() const { return version_; }

			// 添加请求头
			void add_header(const CString& key, const CString& value) { headers.insert(key, value); }

			// 添加请求体内容
			void set_body(const CString& body)
			{
				this->body = body;
				if (body.empty())
					headers.pop(vstr("Content-Length"));
				else
					add_header(vstr("Content-Length"), cstr(body.size()));
			}

			// 设置是否保持连接
			void keep_alive(bool on)
			{
				if (on)
					add_header(vstr("Connection"), vstr("keep-alive"));
				else
					add_header(vstr("Connection"), vstr("close"));
			}

			void __repr__(Buffer& buffer) const
			{
				buffer.adjust_util(body.size() + 1024);
				buffer << method_ << " " << path() << " " << version_ << "\r\n";
				for (auto& [k, v] : headers.items())
					buffer << k << ": " << v << "\r\n";
				buffer << "\r\n" << body;
			}
		};
	}
}
#endif // AYR_NET_HTTP_REQUEST_HPP
