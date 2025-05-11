#ifndef AYR_NET_HTTP_RESPONSE_HPP
#define AYR_NET_HTTP_RESPONSE_HPP

#include "../Socket.hpp"
#include "../../Atring.hpp"
#include "../../Dict.hpp"

namespace ayr
{
	class HttpResponse : public Object<HttpResponse>
	{
		using self = HttpResponse;

		using super = Object<HttpResponse>;
	public:
		Atring version;

		int status_code;

		Atring status_message;

		Dict<Atring, Atring> headers;

		Atring body;

		HttpResponse(const Atring& version, int status_code, const Atring& status_message, bool keep_alive = true) :
			version(version),
			status_code(status_code),
			status_message(status_message),
			headers(),
			body() 
		{
			this->keep_alive(keep_alive);
		}

		HttpResponse(const self& other) :
			version(other.version),
			status_code(other.status_code),
			status_message(other.status_message),
			headers(other.headers),
			body(other.body) {}

		HttpResponse(self&& other) noexcept :
			version(other.version),
			status_code(other.status_code),
			status_message(other.status_message),
			headers(std::move(other.headers)),
			body(other.body) {}

		// 添加一个头
		void add_header(const Atring& key, const Atring& value) { headers.insert(key, value); }

		// 会自动设置 Content-Length 头
		void set_body(const Atring& body)
		{
			this->body = body;
			this->headers.insert("Content-Length", cstr(body.size()));
		};

		// 设置是否保持连接
		void keep_alive(bool on)
		{
			if (on) 
				add_header("Connection", "keep-alive");
			else 
				add_header("Connection", "close");
		}

		Atring text() const
		{
			DynArray<Atring> lines;
			Atring status_code_s = cstr(status_code);
			Atring response_line = " "as.join(arr({ view_of(version), view_of(status_code_s), view_of(status_message) }));
			lines.append(response_line);

			for (auto& [key, value] : headers.items())
				lines.append(": "as.join(arr({ view_of(key), view_of(value) })));
			lines.append("");
			lines.append(body);

			return "\r\n"as.join(lines);
		}
	};
}
#endif // AYR_NET_HTTP_RESPONSE_HPP