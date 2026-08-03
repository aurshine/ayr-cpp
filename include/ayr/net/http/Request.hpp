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
			Atring method_;

			// 请求Uri
			Uri uri_;

			// HTTP版本
			Atring version_;
		public:
			HttpHeaders headers;

			// 已编码的请求体字节。
			CString body;

			HttpRequest() : method_(), uri_(), version_(), headers(), body() {}

			HttpRequest(Atring method, Uri uri, Atring version, HttpHeaders headers = {}, bool keep_alive = true) :
				method_(std::move(method)),
				uri_(std::move(uri)),
				version_(std::move(version)),
				headers(std::move(headers))
			{
				// 请求头默认参数
				if (!this->headers.contains("Host"as) && !uri_.host().empty())
					add_header("Host"as, uri_.host());
				if (!this->headers.contains("Accept"as))
					add_header("Accept"as, "*/*"as);

				// 默认使用https协议
				if (uri_.scheme().empty())
					uri_.scheme("https"as);

				// 根据协议确定端口
				if (uri_.port().empty())
					if (uri_.scheme() == "http"as)
						uri_.port("80"as);
					else if (uri_.scheme() == "https"as)
						uri_.port("443"as);
					else
						ValueError("cannot determine port for scheme");
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
			Atring path() const
			{
				if (uri_.queries().empty())
					return uri_.path();
				return "?"as.join(arr(uri_.path(), uri_.query()));
			}

			// 请求主机名
			const Atring& host() const { return uri_.host(); }

			// 请求的端口
			const Atring& port() const { return uri_.port(); }

			// 添加请求头
			void add_header(const Atring& key, const Atring& value) { headers.insert(key, value); }

			// 添加请求体内容
			void set_body(const CString& body)
			{
				this->body = body;
				if (body.empty())
					headers.pop("Content-Length"as);
				else
					add_header("Content-Length"as, Atring::from(cstr(body.size())));
			}

			// 设置是否保持连接
			void keep_alive(bool on)
			{
				if (on)
					add_header("Connection"as, "keep-alive"as);
				else
					add_header("Connection"as, "close"as);
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
