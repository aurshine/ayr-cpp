#ifndef AYR_NET_HTTP_RESPONSE_HPP
#define AYR_NET_HTTP_RESPONSE_HPP

#include "Headers.hpp"
#include "Uri.hpp"
#include "../Socket.hpp"
#include "../../coro/Generator.hpp"

namespace ayr
{
	namespace net
	{
		struct HttpResponseHead
		{
			int status_code = 0;

			CString version, status_message;

			HttpHeaders headers;
		};

		/**
		* @brief HTTP/1.x 响应增量解析器
		*
		* @details
		* 解析器保存跨多次网络读取的解析状态，可处理固定长度响应体、chunked
		* 响应体以及由连接关闭定界的响应体。每个解析器实例用于解析一个最终响应；
		* 遇到非 101 的 1xx 临时响应时，会自动继续等待后续最终响应。
		 */
		class BodyReader
			{
				// 当前解析器类型别名。
				using self = BodyReader;

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

				// 当前 chunk 的字节数，或固定长度响应体的 Content-Length。
				c_size chunk_size_;

				// 服务端 Socket 对象，流式响应体时用于读取后续数据
				Socket server_;

				// 请求方法用于判定 HEAD 等没有响应体的响应。
				CString request_method_;

				// 响应头在 BodyReader 内完成解析，随后按值交给 HttpResponse。
				HttpResponseHead head_;

				// trailer 属于响应体读取阶段，由 BodyReader 自己保存。
				HttpHeaders trailers_;

				// 对端是否关闭连接
				bool eof;

				// 解析是否结束
				bool done_;

				Buffer resp_buffer;
			public:
				BodyReader(Socket&& server, CString request_method):
					parse_expect(Expect::EXPECT_STATUS_LINE), 
					chunk_size_(0),
					server_(std::move(server)),
					request_method_(std::move(request_method)),
					eof(false),
					done_(false) {}

				BodyReader(const self&) = delete;

				self& operator=(const self&) = delete;

				BodyReader(self&& other) noexcept :
					parse_expect(other.parse_expect),
					chunk_size_(other.chunk_size_),
					server_(std::move(other.server_)),
					request_method_(std::move(other.request_method_)),
					head_(std::move(other.head_)),
					trailers_(std::move(other.trailers_)),
					eof(other.eof),
					done_(other.done_),
					resp_buffer(std::move(other.resp_buffer)) {}

				bool done() const { return done_; }

				const HttpHeaders& trailers() const { return trailers_; }

				// 解析http完整响应
				coro::Task<CString> read()
				{
					if (done()) co_return vstr("");

					// 开始边读边解析HTTP响应
					while (true)
					{
						auto [finish, content] = try_once(resp_buffer);

						// 已经解析完毕 || 解析出内容
						if (finish || !content.empty())
						{
							if (finish)
							{
								done_ = true;
								// HTTP响应已完整解析，此时主动发送TLS close_notify，再由Socket析构
								// 关闭TCP。这样对端可以区分正常结束和被截断的TLS连接。
								net::IoResult shutdown_result = co_await server_.shutdown();
								// close_notify写入底层socket失败意味着关闭通知没有可靠送达，不能
								// 静默当作一次完全成功的HTTPS请求结束。
								if (!shutdown_result.ok())
									RuntimeError(shutdown_result.error);
							}
							co_return std::move(content);
						}
						
						if (!eof)
						{
							// 没解析完且没解析出内容，需要读一次响应数据
							resp_buffer.adjust_util(8192);
							net::IoResult read_result = co_await server_.read(resp_buffer);
							if (!read_result.ok())
								RuntimeError(read_result.error);

							// 对端关闭连接
							eof = read_result.bytes == 0;
						}
						else
						{
							RuntimeError("Incomplete HTTP response: peer closed the connection prematurely");
						}
					}
				}

				// 只解析http状态行和请求头
				coro::Task<HttpResponseHead> parse_head()
				{
					// 开始边读边解析HTTP响应
					while (!try_before_content(resp_buffer))
					{
						if (!eof)
						{
							// 没解析完且没解析出内容，需要读一次响应数据
							resp_buffer.adjust_util(8192);
							net::IoResult read_result = co_await server_.read(resp_buffer);
							if (!read_result.ok())
								RuntimeError(read_result.error);

							// 对端关闭连接
							eof = read_result.bytes == 0;
						}
						else
						{
							RuntimeError("Incomplete HTTP response: peer closed the connection prematurely");
						}
					}
					co_return std::move(head_);
				}

			private:
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
				 * @return 完整响应是否已经解析完成, 是否解析出数据
				 */
				std::pair<bool, CString> try_once(Buffer& buffer)
				{
					while (true)
					{
						switch (parse_expect)
						{
						case Expect::EXPECT_STATUS_LINE:
							if (!parse_status_line(buffer))
								return { incomplete_or_eof("status line"), vstr("") };
							parse_expect = Expect::EXPECT_HEADER;
							break;

						case Expect::EXPECT_HEADER:
						{
							c_size line_size = buffer.find_crlf();
							if (line_size == -1)
								return { incomplete_or_eof("response headers"), vstr("") };

							CString line = consume_line(buffer, line_size);

							// 响应头解析完毕
							if (line.empty())
								select_body_framing();
							else
								parse_header_line(line, false);

							break;
						}
						case Expect::EXPECT_FIXED_BODY:
						{
							auto [ok, content] = parse_fixed_body(buffer);
							if (!ok)
								return { incomplete_or_eof("fixed-length response body"), vstr("") };
							return { true, std::move(content) };
						}
						case Expect::EXPECT_CHUNK_SIZE:
							if (!parse_chunk_size(buffer))
								return { incomplete_or_eof("chunk size"), vstr("") };
							break;

						case Expect::EXPECT_CHUNK_DATA:
						{
							auto [ok, content] = parse_chunk_data(buffer);
							if (!ok)
								return { incomplete_or_eof("chunk data"), vstr("") };
							return { false, std::move(content) };
						}
						case Expect::EXPECT_CHUNK_TRAILER:
						{
							c_size line_size = buffer.find_crlf();
							if (line_size == -1)
								return { incomplete_or_eof("chunk trailers"), vstr("") };

							CString line = consume_line(buffer, line_size);

							if (line.empty())
								return { true, vstr("") };
							else
								parse_header_line(line, true);

							break;
						}
						case Expect::EXPECT_CLOSE_BODY:
							if (!eof)
								return { false, vstr("") };
							return { true, from_buffer(std::move(buffer)) };
						case Expect::DONE:
							return { true, "" };
						}
					}
				}

				// 尝试解析状态行和请求头
				bool try_before_content(Buffer& buffer)
				{
					while (true)
					{
						switch (parse_expect)
						{
						case Expect::EXPECT_STATUS_LINE:
							if (!parse_status_line(buffer))
								return incomplete_or_eof("status line");
							parse_expect = Expect::EXPECT_HEADER;
							break;

						case Expect::EXPECT_HEADER:
						{
							c_size line_size = buffer.find_crlf();
							if (line_size == -1)
								return incomplete_or_eof("response headers");

							CString line = consume_line(buffer, line_size);

							// 响应头解析完毕
							if (line.empty())
								select_body_framing();
							else
								parse_header_line(line, false);

							break;
						}
						default:
							// 解析完状态行和请求头
							return true;
						}
					}
				}
			
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
				static CString consume_line(Buffer& buffer, c_size line_size)
				{
					CString line = dstr(buffer.peek(), line_size);
					buffer.retrieve(line_size + 2);
					return line;
				}

				/**
				 * @brief 统一处理“数据不足”和“EOF 时结构不完整”两种情况
				 *
				 * @param structure 当前尚未解析完整的协议结构名称，用于错误信息
				 *
				 * @return eof 为 false 时固定返回 false，表示需要继续读取
				 *
				 * @throws AyrError eof 为 true 时抛出 ValueError，报告响应被截断
				 */
				bool incomplete_or_eof(const char* structure) const
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
				bool parse_status_line(Buffer& buffer)
				{
					c_size i = buffer.find_crlf();
					if (i == -1) return false;

					CString line = consume_line(buffer, i);
					auto [part1, cnt1] = _char_split(line, ' ');
					auto [part2, cnt2] = _char_split(part1.second, ' ');

					if (cnt2 != 2)
						ValueError(ayr::format("Invalid status line: {}", line));
					if (!part1.first.startswith(vstr("HTTP/")))
						ValueError(ayr::format("Invalid HTTP version: {}", part1.first));

					head_.version = part1.first;
					head_.status_code = valid_status_code(part2.first);
					head_.status_message = part2.second;
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
				static c_size valid_status_code(const CString& value)
				{
					auto status_code = value.toint();
					if (status_code < 100 || status_code > 999)
						ValueError(ayr::format("Invalid HTTP status code: {}", value));
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
				void parse_header_line(const CString& line, bool trailer)
				{
					auto [part, cnt] = _char_split(line, ':');
					if (cnt != 2 || part.first.empty())
						ValueError(ayr::format("Invalid header line: {}", line));

					CString key = part.first.strip();
					CString value = part.second.strip();
					if (key.empty())
						ValueError(ayr::format("Invalid header line: {}", line));
					if (
						trailer
						&& (key.lower() == "content-length" || key.lower() == "transfer-encoding")
						)
						ValueError(ayr::format("Forbidden HTTP trailer field: {}", key));
					if (trailer)
						trailers_.insert(key, value);
					else
						head_.headers.insert(key, value);
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
				static c_size parse_unsigned(const CString& value, int base, const char* field_name)
				{
					if (value.isspace())
						ValueError(ayr::format("Invalid empty HTTP {}.", field_name));

					c_size digit = value.strip().toint(base);

					if (digit < 0)
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
				void select_body_framing()
				{
					// 101 重新开始解析
					if (head_.status_code >= 100 && head_.status_code < 200
						&& head_.status_code != 101)
					{
						head_ = HttpResponseHead();
						parse_expect = Expect::EXPECT_STATUS_LINE;
						return;
					}

					// HEAD、101、204 和 304 没有 HTTP 响应体。
					if (request_method_.upper() == vstr("HEAD")
						|| head_.status_code == 101
						|| head_.status_code == 204
						|| head_.status_code == 304)
					{
						parse_expect = Expect::DONE;
						return;
					}

					bool has_transfer_encoding = head_.headers.contains(vstr("Transfer-Encoding"));
					bool has_content_length = head_.headers.contains(vstr("Content-Length"));
					if (has_transfer_encoding && has_content_length)
						ValueError("HTTP response contains both Transfer-Encoding and Content-Length.");

					if (has_transfer_encoding)
					{
						bool chunked = false;
						for (const auto& [key, value] : head_.headers.items())
						{
							if (key.lower() != "transfer-encoding")
								continue;

							c_size comma_pos = value.rindex(',');
							chunked |= ifelse(
								comma_pos == -1,
								value.lower(),
								value.vslice(comma_pos + 1).lower()
							).strip() == vstr("chunked");
						}

						parse_expect = ifelse(
							chunked,
							Expect::EXPECT_CHUNK_SIZE,
							Expect::EXPECT_CLOSE_BODY
						);
						return;
					}

					if (has_content_length)
					{
						c_size content_length = -1;
						for (const auto& [key, value] : head_.headers.items())
						{
							if (key.lower() != "content-length")
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
				 * @param buffer 保存待消费的响应体字节
				 *
				 * @return 可读字节达到声明长度并成功消费时返回 true，否则返回 false
				 *
				 * @note 完成后仍保留 buffer 中超出 Content-Length 的后续字节。
				 */
				std::pair<bool, CString> parse_fixed_body(Buffer& buffer)
				{
					if (buffer.readable_size() < chunk_size_)
						return { false, "" };
					CString content = dstr(buffer.peek(), chunk_size_);
					buffer.retrieve(chunk_size_);
					return { true, std::move(content) };
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
					CString line = consume_line(buffer, line_size);
					c_size extension = line.index(';');
					CString size_text = ifelse(
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
				 */
				std::pair<bool, CString> parse_chunk_data(Buffer& buffer)
				{
					if (buffer.readable_size() < chunk_size_ + 2)
						return { false, vstr("") };

					const char* bytes = buffer.peek();
					if (bytes[chunk_size_] != '\r' || bytes[chunk_size_ + 1] != '\n')
						ValueError(ayr::format(
							"HTTP chunk data of {} bytes is not followed by CRLF "
							"(next bytes: {}, {}).",
							chunk_size_,
							static_cast<unsigned char>(bytes[chunk_size_]),
							static_cast<unsigned char>(bytes[chunk_size_ + 1])
						));
					CString content = dstr(bytes, chunk_size_);
					buffer.retrieve(chunk_size_ + 2);
					parse_expect = Expect::EXPECT_CHUNK_SIZE;
					return { true, content };
				}
			};

		class HttpResponse
		{
			using self = HttpResponse;

			// 是否是流式响应体
			bool streamed_ = false;

			// 响应解析器
			BodyReader body_reader_;
			
		public:
			// HTTP 版本，如 HTTP/1.1
			CString version;

			// 状态消息，如 OK
			CString status_message;

			// 响应头
			HttpHeaders headers;

			// 原始响应体字节
			CString body;
			
			// 状态码，如 200
			int status_code = 0;

			HttpResponse(Socket&& server, bool streamed, CString request_method):
				streamed_(streamed),
				body_reader_(std::move(server), std::move(request_method)) {}

			HttpResponse(self&& other) noexcept :
				streamed_(other.streamed_),
				body_reader_(std::move(other.body_reader_)),
				version(std::move(other.version)),
				status_code(std::move(other.status_code)),
				status_message(std::move(other.status_message)),
				headers(std::move(other.headers)),
				body(std::move(other.body)) {
			}

			self& operator=(self&& other) noexcept
			{
				if (this == &other) return *this;
				ayr_destroy(this);
				return *ayr_construct(this, std::move(other));
			}

			// 添加一个头
			void add_header(const CString& key, const CString& value) { headers.insert(key, value); }

			// 会自动设置 Content-Length 头
			void set_body(const CString& body)
			{
				this->body = body;
				add_header(vstr("Content-Length"), cstr(this->body.size()));
			};

			// 设置是否保持连接
			void keep_alive(bool on)
			{
				if (on)
					add_header(vstr("Connection"), vstr("keep-alive"));
				else
					add_header(vstr("Connection"), vstr("close"));
			}

			// 非流式传输时等待body读完，流式传输时等待请求头读完
			coro::Task<void> wait()
			{
				HttpResponseHead head = co_await body_reader_.parse_head();
				version = std::move(head.version);
				status_message = std::move(head.status_message);
				headers = std::move(head.headers);
				status_code = head.status_code;

				if (!streamed_ && body.empty())
				{
					Buffer buffer(8192);
					while (!body_reader_.done())
						buffer << co_await body_reader_.read();
					body = from_buffer(std::move(buffer));
				}
			}

			const HttpHeaders& trailers() const { return body_reader_.trailers(); }

			// 接受流式传输的数据
			coro::Generator<coro::Task<CString>> stream()
			{
				if (streamed_)
				{
					while (!body_reader_.done())
						co_yield body_reader_.read();
				}
				co_return coro::finish;
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
	}
}
#endif // AYR_NET_HTTP_RESPONSE_HPP
