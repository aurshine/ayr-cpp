#ifndef AYR_NET_HTTP_REQUEST_HPP
#define AYR_NET_HTTP_REQUEST_HPP

#include "Uri.hpp"
#include "../Socket.hpp"

namespace ayr
{
	namespace net
	{
		class HttpRequest : public Object<HttpRequest>
		{
			using self = HttpRequest;

			using super = Object<HttpRequest>;

			// 请求方法
			Atring method_;

			// 请求Uri
			Uri uri_;

			// HTTP版本
			Atring version_;
		public:
			Dict<Atring, Atring> headers;

			Atring body;

			HttpRequest() : method_(), uri_(), version_(), headers(), body() {}

			HttpRequest(const Atring& method, const Uri& uri, const Atring& version, bool keep_alive = true) :
				method_(method.clone()),
				uri_(uri),
				version_(version.clone())
			{
				if (!uri_.host().empty())
					add_header("Host"as, uri_.host());
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
				method_(other.method_.clone()),
				uri_(other.uri_),
				version_(other.version_.clone()),
				headers(other.headers),
				body(other.body.clone()) {
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
			const Atring& path() const
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
			void add_header(const Atring& key, const Atring& value) { headers.insert(key.clone(), value.clone()); }

			// 添加请求体内容
			void set_body(const Atring& body)
			{
				if (body.empty()) return;
				this->body = body.clone();
				add_header("Content-Length"as, Atring::from_utf8(cstr(body.size())));
			}

			// 设置是否保持连接
			void keep_alive(bool on)
			{
				if (on)
					add_header("Connection"as, "keep-alive"as);
				else
					add_header("Connection"as, "close"as);
			}

			/*
			* 返回请求的文本
			* request line
			* \r\n
			* headers
			* \r\n
			* \r\n
			* body
			*/
			Atring text() const
			{
				DynArray<Atring> lines;
				Atring request_line = " "as.join(arr(method_, path(), version_));
				lines.append(request_line);

				for (auto& [k, v] : headers.items())
					lines.append(": "as.join(arr(k, v)));
				lines.append(""as);
				lines.append(body);

				return "\r\n"as.join(lines);
			}
		};
	}
}
#endif // AYR_NET_HTTP_REQUEST_HPP