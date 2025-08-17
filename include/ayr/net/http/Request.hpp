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

		Dict<Atring, Atring> quries;

		Dict<Atring, Atring> headers;

		Atring body;

		HttpRequest() : method(), uri(), version(), headers(), body() {}

		HttpRequest(const Atring& method, const Atring& uri, const Atring& version, Dict<Atring, Atring> headers, const Atring& body) :
			method(method),
			uri(uri),
			version(version),
			headers(std::move(headers)),
			body(body) {}

		HttpRequest(const self& other) : HttpRequest(other.method, other.uri, other.version, other.headers, other.body) {}

		HttpRequest(self&& other) noexcept : HttpRequest(other.method, other.uri, other.version, std::move(other.headers), other.body) {}

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

		self& operator=(self&& other) noexcept
		{
			if (this == &other) return *this;
			method = std::move(other.method);
			uri = std::move(other.uri);
			version = std::move(other.version);
			headers = std::move(other.headers);
			body = std::move(other.body);
			return *this;
		}

		void set_method(const Atring& method) { this->method = method; }

		void set_uri(const Atring& uri) { this->uri = uri; }

		void set_version(const Atring& version) { this->version = version; }

		void add_header(const Atring& key, const Atring& value) { headers.insert(key, value); }

		void set_body(const Atring& body) { this->body = body; }

		Atring path() const { return uri.split("?", 1)[0]; }

		Dict<Atring, Atring> querys() const
		{
			Array<Atring> splits = uri.split("?", 1);
			Dict<Atring, Atring> querys_dict;

			if (splits.size() == 2)
			{
				Atring querys_string = splits[1];
				for (const Atring& kv_str : querys_string.split("&"))
				{
					Array<Atring> kv = kv_str.split("=", 1);
					if (kv.size() != 2) continue;
					querys_dict.insert(kv[0], kv[1]);
				}
			}

			return querys_dict;
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
			Atring request_line = " "as.join(arr({ view_of(method), view_of(uri), view_of(version) }));
			lines.append(request_line);

			for (auto& [k, v] : headers.items())
				lines.append(": "as.join(arr({ view_of(k), view_of(v) })));
			lines.append("");
			lines.append(body);

			return "\r\n"as.join(lines);
		}
	};

	class RequestParser : public Object<RequestParser>
	{
		enum class Expect
		{
			ExpectReqLine,
			ExpectHeaders,
			ExpectBody,
			Done
		} parse_expect;

		Atring buffer;
	public:
		RequestParser() : parse_expect(Expect::ExpectReqLine), buffer() {}

		bool operator()(HttpRequest& request, const Atring& data)
		{
			buffer += data;

			// 是否还有更多数据可解析
			bool has_more = true;
			while (has_more)
			{
				switch (parse_expect)
				{
				case Expect::ExpectReqLine:
					has_more = parse_req_line(request);
					break;
				case Expect::ExpectHeaders:
					has_more = parse_headers(request);
					break;
				case Expect::ExpectBody:
					parse_body(request);
					has_more = false;
					break;
				}
			}

			if (parse_expect == Expect::Done)
			{
				parse_expect = Expect::ExpectReqLine;
				return true;
			}

			return false;
		}
	private:
		bool parse_req_line(HttpRequest& request)
		{
			c_size pos = buffer.find("\r\n");
			if (pos == -1) return false;

			Atring req_line = buffer.slice(0, pos);
			buffer = buffer.slice(pos + 2);

			Array<Atring> req_datas = req_line.split(" ", 3);
			if (req_datas.size() != 3)
				RuntimeError("parse request line failed, req_datas.size() != 3");

			request.set_method(std::move(req_datas[0]));
			request.set_uri(std::move(req_datas[1]));
			request.set_version(std::move(req_datas[2]));

			parse_expect = Expect::ExpectHeaders;
			return true;
		}

		bool parse_headers(HttpRequest& request)
		{
			c_size pos = buffer.find("\r\n");
			if (pos == -1) return false;
			if (pos == 0)
			{
				parse_expect = Expect::ExpectBody;
				buffer = buffer.slice(pos + 2);
				return true;
			}

			Atring header_line = buffer.slice(0, pos);
			buffer = buffer.slice(pos + 2);
			Array<Atring> kv = header_line.split(":", 1);
			if (kv.size() != 2)
				RuntimeError("parse headers failed, kv.size() != 2");

			request.add_header(std::move(kv[0].strip()), std::move(kv[1].strip()));

			return true;
		}

		void parse_body(HttpRequest& request)
		{
			c_size content_length = request.headers.get("content-length", "0").to_int();
			if (buffer.size() >= content_length)
			{
				request.set_body(std::move(buffer.slice(0, content_length)));
				buffer = buffer.slice(content_length);

				parse_expect = Expect::Done;
			}
		}
	};
}
#endif // AYR_NET_HTTP_REQUEST_HPP