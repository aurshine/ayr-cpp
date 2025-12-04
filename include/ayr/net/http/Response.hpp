#ifndef AYR_NET_HTTP_RESPONSE_HPP
#define AYR_NET_HTTP_RESPONSE_HPP

#include "Uri.hpp"
#include "../Socket.hpp"

namespace ayr
{
	namespace net
	{
		class HttpResponse
		{
			using self = HttpResponse;
		public:
			// HTTP 版本，如 HTTP/1.1
			Atring version;

			// 状态码，如 200
			Atring status_code;

			// 状态消息，如 OK
			Atring status_message;

			// 响应头
			Dict<Atring, Atring> headers;

			// 响应体
			Atring body;

			HttpResponse() {}

			HttpResponse(const self& other) :
				version(other.version.clone()),
				status_code(other.status_code.clone()),
				status_message(other.status_message.clone()),
				headers(other.headers),
				body(other.body.clone()) {
			}

			HttpResponse(self&& other) noexcept :
				version(std::move(other.version)),
				status_code(std::move(other.status_code)),
				status_message(std::move(other.status_message)),
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

			// 添加一个头
			void add_header(const Atring& key, const Atring& value) { headers.insert(key.clone(), value.clone()); }

			// 会自动设置 Content-Length 头
			void set_body(const Atring& body)
			{
				if (body.empty()) return;
				this->body = body.clone();
				add_header("Content-Length"as, Atring::from_utf8(cstr(body.size())));
			};

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
				buffer.expand_util(body.size() + 1024);
				buffer << version << " " << status_code << " " << status_message << "\r\n";
				for (auto& [key, value] : headers.items())
					buffer << key << ": " << value << "\r\n";
				buffer << "\r\n" << body;
			}
		};

		class ResponseParser
		{
			using self = ResponseParser;

			// 期望解析什么结构
			enum class Expect
			{
				EXPECT_STATUS_LINE,
				EXPECT_HEADER,
				EXPECT_BODY,
				DONE
			}parse_expect;
		public:
			ResponseParser() : parse_expect(Expect::EXPECT_STATUS_LINE) {}

			bool operator()(HttpResponse& response, Buffer& buffer)
			{
				switch (parse_expect)
				{
				case Expect::EXPECT_STATUS_LINE:
					if (parse_status_line(response, buffer))
						parse_expect = Expect::EXPECT_HEADER;
					else
						break;
				case Expect::EXPECT_HEADER:
					if (parse_header(response, buffer))
						parse_expect = Expect::EXPECT_BODY;
					else
						break;
				case Expect::EXPECT_BODY:
					if (parse_body(response, buffer))
						parse_expect = Expect::DONE;
				}

				if (parse_expect == Expect::DONE)
				{
					// 重置
					parse_expect = Expect::EXPECT_STATUS_LINE;
					return true;
				}
				return false;
			}

			bool parse_status_line(HttpResponse& response, Buffer& buffer)
			{
				c_size i = buffer.find_crlf();
				if (i == -1) return false;

				Atring line = Atring::from(vstr(buffer.peek(), i));
				buffer.retrieve(i + 2);

				Array<Atring> parts = line.split(" "as, 2);
				if (parts.size() != 3)
					ValueError("Invalid status line: "as + line);

				response.version = parts[0].clone();
				response.status_code = parts[1].clone();
				response.status_message = parts[2].clone();
				return true;
			}

			bool parse_header(HttpResponse& response, Buffer& buffer)
			{
				while (true)
				{
					c_size i = buffer.find_crlf();
					if (i == -1) return false;

					Atring line = Atring::from(vstr(buffer.peek(), i));
					buffer.retrieve(i + 2);

					if (line.empty())
						return true;

					Array<Atring> parts = line.split(":"as, 1);
					if (parts.size() != 2)
						ValueError("Invalid header line: "as + line);
					response.add_header(parts[0].clone(), parts[1].strip().clone());
				}
			}

			bool parse_body(HttpResponse& response, Buffer& buffer)
			{
				if (!response.headers.contains("Content-Length"as))
					return true;
				c_size content_length = response.headers.get("Content-Length"as).to_int();

				if (buffer.readable_size() < content_length)
					return false;
				response.body = Atring::from(vstr(buffer.peek(), content_length));
				buffer.retrieve(content_length);
				return true;
			}
		};
	}
}
#endif // AYR_NET_HTTP_RESPONSE_HPP