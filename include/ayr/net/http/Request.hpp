#ifndef AYR_NET_HTTP_REQUEST_HPP
#define AYR_NET_HTTP_REQUEST_HPP

#include "../Socket.hpp"
#include "../../Atring.hpp"
#include "../../Dict.hpp"
#include "../../base/View.hpp"

namespace ayr
{
	class HttpRequest : public Object<HttpRequest>
	{
		using self = HttpRequest;

		using super = Object<HttpRequest>;

	public:
		Atring method, uri, version;

		Dict<Atring, Atring> headers;

		Atring body;

		HttpRequest(Atring method, Atring uri, Atring version, Dict<Atring, Atring> headers, Atring body) :
			method(method), uri(uri), version(version), headers(headers), body(body) {}

		HttpRequest(const self& other) : HttpRequest(other.method, other.uri, other.version, other.headers, other.body) {}

		HttpRequest(self&& other) : HttpRequest(std::move(other.method), std::move(other.uri), std::move(other.version), std::move(other.headers), std::move(other.body)) {}

		self& operator=(const self& other)
		{
			if (this == &other) return *this;
			method = other.method;
			uri = other.uri;
			version = other.version;
			headers = other.headers;
			body = other.body;
			return *this;
		}

		self& operator=(self&& other)
		{
			if (this == &other) return *this;
			method = std::move(other.method);
			uri = std::move(other.uri);
			version = std::move(other.version);
			headers = std::move(other.headers);
			body = std::move(other.body);
			return *this;
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
			Atring request_line = Atring::ajoin(arr({ view_of(method), view_of(uri), view_of(version) }));
			DynArray<Atring> kvs;
			for (auto& [k, v] : headers.items())
				kvs.append(": "as.join(arr({ view_of(k), view_of(v) })));
			Atring headers_lines = "\n"as.join(kvs);
			Atring null_line = "";

			return "\r\n"as.join(arr({ view_of(request_line), view_of(headers_lines), view_of(null_line), view_of(body) }));
		}
	};

	class RequestParser : public Object<RequestParser>
	{
	public:
		RequestParser() {}

		HttpRequest operator()(Atring data)
		{
			Dict<Atring, Atring> headers;

			Atring method, uri, version;

			parse_req_line(data, method, uri, version);
			parse_headers(data, headers);

			/*c_size content_length = headers.get("content-length", "0").to_int();
			if (data.size() != content_length)
			RuntimeError("parse request body failed, content_length != body.size()");*/

			return HttpRequest(std::move(method), std::move(uri), std::move(version), std::move(headers), std::move(data));
		}
	private:
		void parse_req_line(Atring& data, Atring& method, Atring& uri, Atring& version)
		{
			c_size pos = data.find("\r\n");
			if (pos == -1)
				RuntimeError("parse request line failed, not found \\r\\n");

			Array<Atring> req_datas = data.slice(0, pos).split(" ");
			if (req_datas.size() != 3)
				RuntimeError("parse request line failed, req_datas.size() != 3");

			method = std::move(req_datas[0]);
			uri = std::move(req_datas[1]);
			version = std::move(req_datas[2]);

			data = data.slice(pos + 2);
		}

		void parse_headers(Atring& data, Dict<Atring, Atring>& headers)
		{
			c_size pos = data.find("\r\n\r\n");
			if (pos == -1) RuntimeError("parse headers failed, not found \\r\\n\\r\\n");

			Atring headers_datas = data.slice(0, pos);
			for (const Atring& kv_str : headers_datas.split("\r\n"))
			{
				c_size pos = kv_str.find(":");
				if (pos == -1) RuntimeError("parse headers failed, not found :");
				headers.insert(kv_str.slice(0, pos).lower().strip(), kv_str.slice(pos + 1).lower().strip());
			}

			data = data.slice(pos + 4);
		}
	};
}
#endif // AYR_NET_HTTP_REQUEST_HPP