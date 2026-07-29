#ifndef AYR_NET_HTTP_RESPONSE_HPP
#define AYR_NET_HTTP_RESPONSE_HPP

#include "Headers.hpp"
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

			// 状态消息，如 OK
			Atring status_message;

			// 响应头
			HttpHeaders headers;

			// 原始响应体字节
			CString body;
			
			// 状态码，如 200
			int status_code = 0;

			HttpResponse() {}

			HttpResponse(const self& other) :
				version(other.version),
				status_code(other.status_code),
				status_message(other.status_message),
				headers(other.headers),
				body(other.body) {
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
			void add_header(const Atring& key, const Atring& value) { headers.insert(key, value); }

			// 会自动设置 Content-Length 头
			void set_body(const CString& body)
			{
				this->body = body;
				add_header("Content-Length"as, Atring::from(cstr(this->body.size())));
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
				buffer.adjust_util(body.size() + 1024);
				buffer << version << " " << status_code << " " << status_message << "\r\n";
				for (auto& [key, value] : headers.items())
					buffer << key << ": " << value << "\r\n";
				buffer << "\r\n" << body;
			}
		};

		/**
		 * @brief HTTP/1.x 响应增量解析器
		 *
		 * @details
		 * 解析器保存跨多次网络读取的解析状态，可处理固定长度响应体、chunked
		 * 响应体以及由连接关闭定界的响应体。每个解析器实例用于解析一个最终响应；
		 * 遇到非 101 的 1xx 临时响应时，会自动继续等待后续最终响应。
		 */
		class ResponseParser
		{
			// 当前解析器类型别名。
			using self = ResponseParser;

			/**
			 * @brief 响应解析状态
			 *
			 * @details 每个枚举值表示下一步必须从输入缓冲区读取的协议结构。
			 */
			enum class Expect
			{
				// 等待解析响应首行：HTTP 版本、状态码和状态消息。
				EXPECT_STATUS_LINE,

				// 等待解析普通响应头，直到遇到空行为止。
				EXPECT_HEADER,

				// 根据 Content-Length 等待固定字节数的响应体。
				EXPECT_FIXED_BODY,

				// 等待解析 chunked 编码中以十六进制表示的 chunk 长度行。
				EXPECT_CHUNK_SIZE,

				// 等待读取当前 chunk 的数据及其后的 CRLF。
				EXPECT_CHUNK_DATA,

				// 已读到零长度 chunk，等待解析 trailer 字段及结尾空行。
				EXPECT_CHUNK_TRAILER,

				// 响应体没有显式长度，持续读取直到对端关闭连接。
				EXPECT_CLOSE_BODY,

				// 最终响应已经完整解析，后续调用直接返回成功。
				DONE
			};

			// 当前解析状态，决定下一次从输入缓冲区解析哪种结构。
			Expect parse_expect;

			// 发起请求时使用的方法，用于识别 HEAD 等不应包含响应体的请求。
			Atring request_method_;

			// 累积 chunked 或连接关闭定界的响应体原始字节。
			Buffer body_buffer_;

			// 当前 chunk 的字节数，或固定长度响应体的 Content-Length。
			c_size chunk_size_;
		public:
			/**
			 * @brief 创建一个 HTTP 响应增量解析器
			 *
			 * @param request_method 发起请求的方法；HEAD 响应据此判定为无响应体
			 *
			 * @param max_header_size 状态行、响应头和 trailer 的最大累计字节数
			 *
			 * @param max_body_size 响应体允许占用的最大字节数
			 */
			ResponseParser(
				const Atring& request_method = {},
				c_size max_header_size = 64 * 1024,
				c_size max_body_size = 64 * 1024 * 1024
			) :
				parse_expect(Expect::EXPECT_STATUS_LINE),
				request_method_(request_method),
				body_buffer_(),
				chunk_size_(0){}

			/**
			 * @brief 增量解析 HTTP 响应
			 *
			 * @details
			 * 尽可能消费 buffer 中的完整协议结构。数据不足时保留当前状态并返回
			 * false，调用方可追加网络数据后再次调用；若 eof 为 true 但当前结构仍
			 * 不完整，则抛出 ValueError，避免把截断响应当作成功响应。
			 *
			 * @param response 保存解析结果
			 *
			 * @param buffer 保存尚未消费的网络字节
			 *
			 * @param eof 对端是否已经关闭读取方向
			 *
			 * @return 完整响应是否已经解析完成
			 */
			bool operator()(HttpResponse& response, Buffer& buffer, bool eof = false)
			{
				while (true)
				{
					switch (parse_expect)
					{
					case Expect::EXPECT_STATUS_LINE:
						if (!parse_status_line(response, buffer))
							return incomplete_or_eof(eof, "status line");
						parse_expect = Expect::EXPECT_HEADER;
						break;

					case Expect::EXPECT_HEADER:
					{
						c_size line_size = buffer.find_crlf();
						if (line_size == -1)
							return incomplete_or_eof(eof, "response headers");

						Atring line = consume_line(buffer, line_size);
						
						// 响应头解析完毕
						if (line.empty())
							select_body_framing(response);
						else
							parse_header_line(response, line, false);
						
						break;
					}
					case Expect::EXPECT_FIXED_BODY:
						if (!parse_fixed_body(response, buffer))
							return incomplete_or_eof(eof, "fixed-length response body");
						parse_expect = Expect::DONE;
						break;

					case Expect::EXPECT_CHUNK_SIZE:
						if (!parse_chunk_size(buffer))
							return incomplete_or_eof(eof, "chunk size");
						break;

					case Expect::EXPECT_CHUNK_DATA:
						if (!parse_chunk_data(buffer))
							return incomplete_or_eof(eof, "chunk data");
						break;

					case Expect::EXPECT_CHUNK_TRAILER:
					{
						c_size line_size = buffer.find_crlf();
						if (line_size == -1)
							return incomplete_or_eof(eof, "chunk trailers");
						
						Atring line = consume_line(buffer, line_size);
						
						if (line.empty())
							finish_buffered_body(response);
						else
							parse_header_line(response, line, true);

						break;
					}
					case Expect::EXPECT_CLOSE_BODY:
						append_close_body(buffer);
						if (!eof)
							return false;
						finish_buffered_body(response);
						break;

					case Expect::DONE:
						return true;
					}
				}
			}
		private:
			/**
			 * @brief 读取并消费一行不含行尾 CRLF 的 UTF-8 文本
			 *
			 * @param buffer 保存完整文本行及其 CRLF 的输入缓冲区
			 *
			 * @param line_size CRLF 之前的字节数
			 *
			 * @return 解码后的文本行
			 *
			 * @note 调用成功后会从 buffer 中同时移除文本内容和两个 CRLF 字节。
			 */
			static Atring consume_line(Buffer& buffer, c_size line_size)
			{
				Atring line = Atring::from_utf8(vstr(buffer.peek(), line_size));
				buffer.retrieve(line_size + 2);
				return line;
			}

			/**
			 * @brief 统一处理“数据不足”和“EOF 时结构不完整”两种情况
			 *
			 * @param eof 对端是否已经关闭读取方向
			 *
			 * @param structure 当前尚未解析完整的协议结构名称，用于错误信息
			 *
			 * @return eof 为 false 时固定返回 false，表示需要继续读取
			 *
			 * @throws AyrError eof 为 true 时抛出 ValueError，报告响应被截断
			 */
			bool incomplete_or_eof(bool eof, const char* structure) const
			{
				if (eof)
					ValueError(ayr::format("Unexpected EOF while parsing HTTP {}.", structure));
				return false;
			}

			/**
			 * @brief 解析 HTTP 响应状态行
			 *
			 * @details 验证状态行包含 HTTP 版本、三位状态码和状态消息，并把结果
			 * 写入 response。状态行尚未完整到达时不会消费任何输入。
			 *
			 * @param response 保存解析后的版本、状态码和状态消息
			 *
			 * @param buffer 保存待解析网络字节
			 *
			 * @return 状态行完整并成功解析时返回 true，否则返回 false
			 */
			bool parse_status_line(HttpResponse& response, Buffer& buffer)
			{
				c_size i = buffer.find_crlf();
				if (i == -1) return false;

				Atring line = consume_line(buffer, i);
				Array<Atring> parts = line.split(" "as, 2);
				if (parts.size() != 3)
					ValueError(vstr("Invalid status line: ") + cstr(line));
				if (!parts[0].startswith("HTTP/"as))
					ValueError(vstr("Invalid HTTP version: ") + cstr(parts[0]));
				
				response.version = parts[0];
				response.status_code = valid_status_code(parts[1]);
				response.status_message = parts[2];
				return true;
			}

			/**
			 * @brief 验证并转换三位 HTTP 状态码
			 *
			 * @param value 状态行中的状态码文本
			 *
			 * @return 状态码对应的整数
			 *
			 * @throws AyrError value 不是恰好三位十进制数字时抛出 ValueError
			 */
			static c_size valid_status_code(const Atring& value)
			{
				auto [status_code, remain] = value.toint();
				if (status_code < 100 || status_code > 999 || !remain.empty())
					ValueError(vstr("Invalid HTTP status code: ") + cstr(value));
				return status_code;
			}

			/**
			 * @brief 解析一个响应头或 trailer 字段
			 *
			 * @param response 接收解析后字段的响应对象
			 *
			 * @param line 不包含 CRLF 的完整字段行
			 *
			 * @param trailer true 表示当前字段属于 chunk trailer
			 *
			 * @note trailer 中禁止出现 Content-Length 和 Transfer-Encoding。
			 */
			static void parse_header_line(HttpResponse& response, const Atring& line, bool trailer)
			{
				Array<Atring> parts = line.split(":"as, 1);
				if (parts.size() != 2 || parts[0].empty())
					ValueError(vstr("Invalid header line: ") + cstr(line));
				Atring key = parts[0].strip();
				Atring value = parts[1].strip();
				if (key.empty())
					ValueError(vstr("Invalid header line: ") + cstr(line));
				if (
					trailer
					&& (key.lower() == "content-length"as || key.lower() == "transfer-encoding"as)
				)
					ValueError(vstr("Forbidden HTTP trailer field: ") + cstr(key));
				response.headers.insert(key, value);
			}

			/**
			 * @brief 严格解析指定进制的无符号整数
			 *
			 * @param value 待解析文本，允许首尾空白
			 *
			 * @param base 进制；Content-Length 使用 10，chunk 长度使用 16
			 *
			 * @param field_name 字段名称，仅用于构造错误信息
			 *
			 * @return 转换后的非负整数
			 *
			 * @throws AyrError 文本为空、包含非法数字或数值溢出时抛出 ValueError
			 */
			static c_size parse_unsigned(const Atring& value, int base, const char* field_name)
			{
				if (value.isspace())
					ValueError(ayr::format("Invalid empty HTTP {}.", field_name));

				auto [digit, remain] = value.strip().toint(base);

				if (!remain.empty() || digit < 0)
					ValueError(ayr::format("Invalid HTTP {}: {}.", field_name, value));
				
				return digit;
			}

			/**
			 * @brief 根据请求方法、状态码和响应头选择响应体定界方式
			 *
			 * @param response 已完成状态行和响应头解析的响应对象
			 *
			 * @details
			 * ；Transfer-Encoding 最终编码为
			 * chunked 时进入 chunk 状态；Content-Length 进入固定长度状态；其余
			 * 响应进入连接关闭定界状态。同时拒绝同时携带 Transfer-Encoding 与
			 * Content-Length 的歧义响应。
			 */
			void select_body_framing(HttpResponse& response)
			{
				// 101 重新开始解析
				if (response.status_code == 101)
				{
					response = HttpResponse();
					parse_expect = Expect::EXPECT_STATUS_LINE;
					return;
				}

				// HEAD、1xx、204 和 304 没有响应体，直接进入 DONE
				if (request_method_.upper() == "HEAD"as
					|| (response.status_code >= 100 && response.status_code < 200)
					|| response.status_code == 204
					|| response.status_code == 304)
				{
					response.body = CString();
					parse_expect = Expect::DONE;
					return;
				}

				bool has_transfer_encoding = response.headers.contains("Transfer-Encoding"as);
				bool has_content_length = response.headers.contains("Content-Length"as);
				if (has_transfer_encoding && has_content_length)
					ValueError("HTTP response contains both Transfer-Encoding and Content-Length.");

				if (has_transfer_encoding)
				{
					Atring final_coding;
					for (const auto& [key, value] : response.headers.items())
					{
						if (key.lower() != "transfer-encoding"as)
							continue;
						for (const Atring& coding : value.split(","as))
						{
							Atring normalized = coding.strip().lower();
							if (normalized.empty())
								ValueError("Invalid empty HTTP transfer coding.");
							final_coding = std::move(normalized);
						}
					}
					parse_expect = ifelse(
						final_coding == "chunked"as,
						Expect::EXPECT_CHUNK_SIZE,
						Expect::EXPECT_CLOSE_BODY
					);
					return;
				}

				if (has_content_length)
				{
					c_size content_length = -1;
					for (const auto& [key, value] : response.headers.items())
					{
						if (key.lower() != "content-length"as)
							continue;
						c_size parsed_length = parse_unsigned(value, 10, "Content-Length");
						if (content_length != -1 && content_length != parsed_length)
							ValueError("HTTP response contains conflicting Content-Length fields.");
						content_length = parsed_length;
					}
					
					chunk_size_ = content_length;
					parse_expect = Expect::EXPECT_FIXED_BODY;
					return;
				}

				// 没有显式长度时，响应体由对端关闭连接定界。
				parse_expect = Expect::EXPECT_CLOSE_BODY;
			}

			/**
			 * @brief 解析 Content-Length 定界的固定长度响应体
			 *
			 * @param response 保存完整响应体
			 *
			 * @param buffer 保存待消费的响应体字节
			 *
			 * @return 可读字节达到声明长度并成功消费时返回 true，否则返回 false
			 *
			 * @note 完成后仍保留 buffer 中超出 Content-Length 的后续字节。
			 */
			bool parse_fixed_body(HttpResponse& response, Buffer& buffer)
			{
				if (buffer.readable_size() < chunk_size_)
					return false;
				response.body = vstr(buffer.peek(), chunk_size_).clone();
				buffer.retrieve(chunk_size_);
				return true;
			}

			/**
			 * @brief 解析 chunked 编码的 chunk 长度行
			 *
			 * @param buffer 保存 chunk 长度行及后续字节
			 *
			 * @return 长度行完整并成功解析时返回 true，否则返回 false
			 *
			 * @details 支持分号后的 chunk extension；零长度 chunk 会转入 trailer
			 * 状态，非零长度 chunk 会转入数据状态。
			 */
			bool parse_chunk_size(Buffer& buffer)
			{
				
				c_size line_size = buffer.find_crlf();
				if (line_size == -1)
					return false;
				Atring line = consume_line(buffer, line_size);
				c_size extension = line.index(";"as);
				Atring size_text = ifelse(
					extension == -1,
					line.strip(),
					line.slice(0, extension).strip()
				);
				chunk_size_ = parse_unsigned(size_text, 16, "chunk size");
				
				parse_expect = ifelse(
					chunk_size_ == 0,
					Expect::EXPECT_CHUNK_TRAILER,
					Expect::EXPECT_CHUNK_DATA
				);
				return true;
			}

			/**
			 * @brief 读取当前 chunk 数据并验证结尾 CRLF
			 *
			 * @param buffer 保存 chunk 数据、结尾 CRLF 及可能的后续 chunk
			 *
			 * @return 当前 chunk 数据及 CRLF 已完整到达时返回 true，否则返回 false
			 *
			 * @note 成功后数据追加到 body_buffer_，解析状态重新切换到 chunk 长度行。
			 */
			bool parse_chunk_data(Buffer& buffer)
			{
				if (buffer.readable_size() < chunk_size_ + 2)
					return false;
				const char* bytes = buffer.peek();
				if (bytes[chunk_size_] != '\r' || bytes[chunk_size_ + 1] != '\n')
					ValueError(ayr::format(
						"HTTP chunk data of {} bytes is not followed by CRLF "
						"(next bytes: {}, {}).",
						chunk_size_,
						static_cast<unsigned char>(bytes[chunk_size_]),
						static_cast<unsigned char>(bytes[chunk_size_ + 1])
					));
				body_buffer_.append_bytes(bytes, chunk_size_);
				buffer.retrieve(chunk_size_ + 2);
				parse_expect = Expect::EXPECT_CHUNK_SIZE;
				return true;
			}

			/**
			 * @brief 累积由连接关闭定界的响应体字节
			 *
			 * @param buffer 保存本次网络读取到的剩余响应体字节
			 *
			 * @note 函数会消费 buffer 中全部可读字节，并检查响应体大小上限。
			 */
			void append_close_body(Buffer& buffer)
			{
				body_buffer_.append_bytes(buffer.peek(), buffer.readable_size());
				buffer.retrieve(buffer.readable_size());
			}

			/**
			 * @brief 将累计响应体转移到响应对象并标记解析完成
			 *
			 * @param response 接收 body_buffer_ 所有权的响应对象
			 *
			 * @note 转移完成后会重新初始化内部缓冲区并把状态切换为 DONE。
			 */
			void finish_buffered_body(HttpResponse& response)
			{
				response.body = from_buffer(std::move(body_buffer_));
				body_buffer_.clear();
				parse_expect = Expect::DONE;
			}
		};
	}
}
#endif // AYR_NET_HTTP_RESPONSE_HPP
