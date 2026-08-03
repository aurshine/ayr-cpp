#include <ayr/net/http.hpp>
#include <ayr/base/utest.hpp>

using namespace ayr;

coro::Task<void> internet(coro::IoContext* io_context, net::Uri uri, net::HttpHeaders headers)
{
	auto resp = co_await net::get(io_context, uri, headers);
	UTEST_EXPECT_EQ(resp.status_code, 200);
}

int main()
{
	static_assert(!std::is_copy_constructible_v<net::Session>);
	static_assert(std::is_move_constructible_v<net::Session>);

	UTEST_SCOPE("测试 URI 解析和查询参数访问。")
	{
		net::Uri full = net::uri("https://example.com:8443/api/v1?q=openai&page=2#top");
		UTEST_EXPECT_EQ(full.scheme(), "https");
		UTEST_EXPECT_EQ(full.host(), "example.com");
		UTEST_EXPECT_EQ(full.port(), 8443);
		UTEST_EXPECT_EQ(full.path(), "/api/v1");
		UTEST_EXPECT_EQ(full.queries().get("q"), "openai");
		UTEST_EXPECT_EQ(full.queries().get("page"), "2");
		UTEST_EXPECT_EQ(full.fragment(), "top");

		net::Uri fragment_only = net::uri("https://example.com/path#frag");
		UTEST_EXPECT_EQ(fragment_only.fragment(), "frag");

		net::Uri relative = net::uri("/search?q=cpp#result");
		UTEST_EXPECT_EQ(relative.host(), "");
		UTEST_EXPECT_EQ(relative.path(), "/search");
		UTEST_EXPECT_EQ(relative.queries().get("q"), "cpp");
		UTEST_EXPECT_EQ(relative.fragment(), "result");

		net::Uri bare_query = net::uri("https://example.com/path?flag=1&empty=2#frag");
		UTEST_EXPECT_EQ(bare_query.queries().get("flag"), "1");
		UTEST_EXPECT_EQ(bare_query.queries().get("empty"), "2");
	};

	UTEST_SCOPE("测试 HTTP 请求构造、序列化和头部校验。")
	{
		net::HttpRequest request(
			"POST",
			net::uri("https://example.com:8443/search?q=cpp"),
			"HTTP/1.1"
		);
		request.set_body("你好");

		UTEST_EXPECT_EQ(request.path(), "/search?q=cpp");
		UTEST_EXPECT_EQ(request.headers.get("host"), "example.com");
		UTEST_EXPECT_EQ(request.headers.get("CONTENT-LENGTH"), "6");
		UTEST_EXPECT_EQ(request.body, "你好");

		Buffer serialized;
		serialized << request;
		CString request_bytes = vstr(serialized.peek(), serialized.readable_size());
		UTEST_EXPECT(request_bytes.contains("Content-Length: 6\r\n"));
		UTEST_EXPECT(request_bytes.endswith("你好"));

		request.set_body("");
		UTEST_EXPECT(request.body.empty());
		UTEST_EXPECT(!request.headers.contains("content-length"));
		UTEST_EXPECT_AYR_ERROR(request.add_header("Bad Header", "value"));
		UTEST_EXPECT_AYR_ERROR(request.add_header("X-Test", "one\r\ntwo"));

		net::Uri manual_uri;
		manual_uri.scheme("HTTP");
		manual_uri.host("example.com");
		manual_uri.port(8080);
		net::HttpRequest manual_request("GET", manual_uri, "HTTP/1.1");
		UTEST_EXPECT_EQ(manual_request.uri().scheme(), "http");
		tlog(manual_request.path());
		UTEST_EXPECT_EQ(manual_request.path(), "/");
	};

	UTEST_SCOPE("测试固定长度 HTTP 响应的增量解析。")
	{
		net::HttpResponse response;
		net::ResponseParser parser("GET");
		Buffer buffer;
		buffer <<
			"HTTP/1.1 200 OK\r\n"
			"content-length: 5\r\n"
			"Set-Cookie: first=1\r\n"
			"set-cookie: second=2\r\n"
			"X-Test: one\r\n\r\nhe";

		UTEST_EXPECT(!parser(response, buffer));
		buffer << "lloNEXT";
		UTEST_EXPECT(parser(response, buffer));
		UTEST_EXPECT_EQ(response.status_code, 200);
		UTEST_EXPECT_EQ(response.headers.get("Content-Length"), "5");
		UTEST_EXPECT_EQ(response.body, "hello");
		UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), "NEXT");
	};

	UTEST_SCOPE("测试 chunked HTTP 响应的增量解析。")
	{
		net::HttpResponse response;
		net::ResponseParser parser("GET");
		Buffer buffer;
		buffer << "HTTP/1.1 200 OK\r\nTransfer-Encoding: Chunked\r\n\r\n4\r\nWi";

		UTEST_EXPECT(!parser(response, buffer));
		UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), "Wi");

		buffer << "ki\r\n5;name=value\r\npedia\r\n0\r\nX-Trailer: done\r\n\r\n";
		UTEST_EXPECT(parser(response, buffer));
		UTEST_EXPECT_EQ(response.body, "Wikipedia");
		UTEST_EXPECT_EQ(response.headers.get("x-trailer"), "done");
		UTEST_EXPECT(buffer.readable_size() == 0);
	};

	UTEST_SCOPE("测试关闭分隔响应和无正文响应。")
	{
		net::HttpResponse close_response;
		net::ResponseParser close_parser("GET");
		Buffer close_buffer;
		close_buffer << "HTTP/1.1 200 OK\r\nConnection: close\r\n\r\nclose body";

		UTEST_EXPECT(!close_parser(close_response, close_buffer));
		UTEST_EXPECT(close_parser(close_response, close_buffer, true));
		UTEST_EXPECT_EQ(close_response.body, "close body");

		net::HttpResponse no_body_response;
		net::ResponseParser no_body_parser("GET");
		Buffer no_body_buffer;
		no_body_buffer << "HTTP/1.1 204 No Content\r\nDate: now\r\n\r\n";
		UTEST_EXPECT(no_body_parser(no_body_response, no_body_buffer));
		UTEST_EXPECT(no_body_response.body.empty());

		net::HttpResponse head_response;
		net::ResponseParser head_parser("HEAD");
		Buffer head_buffer;
		head_buffer << "HTTP/1.1 200 OK\r\nContent-Length: 123\r\n\r\n";
		UTEST_EXPECT(head_parser(head_response, head_buffer));
		UTEST_EXPECT(head_response.body.empty());
	};

	UTEST_SCOPE("测试临时响应和非法响应处理。")
	{
		net::HttpResponse interim_response;
		net::ResponseParser interim_parser("POST");
		Buffer interim_buffer;
		interim_buffer <<
			"HTTP/1.1 101 Continue\r\n\r\n"
			"HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK";
		UTEST_EXPECT(interim_parser(interim_response, interim_buffer));
		UTEST_EXPECT_EQ(interim_response.status_code, 200);
		UTEST_EXPECT_EQ(interim_response.body, "OK");

		net::HttpResponse truncated_response;
		net::ResponseParser truncated_parser("GET");
		Buffer truncated_buffer;
		truncated_buffer << "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nabc";
		UTEST_EXPECT(!truncated_parser(truncated_response, truncated_buffer));
		UTEST_EXPECT_AYR_ERROR(truncated_parser(truncated_response, truncated_buffer, true));

		net::HttpResponse ambiguous_response;
		net::ResponseParser ambiguous_parser("GET");
		Buffer ambiguous_buffer;
		ambiguous_buffer <<
			"HTTP/1.1 200 OK\r\n"
			"Content-Length: 1\r\n"
			"Transfer-Encoding: chunked\r\n\r\n";
		UTEST_EXPECT_AYR_ERROR(ambiguous_parser(ambiguous_response, ambiguous_buffer));

		net::HttpResponse invalid_chunk_response;
		net::ResponseParser invalid_chunk_parser("GET");
		Buffer invalid_chunk_buffer;
		invalid_chunk_buffer <<
			"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n"
			"3\r\nabcXX";
		UTEST_EXPECT_AYR_ERROR(invalid_chunk_parser(
			invalid_chunk_response,
			invalid_chunk_buffer
		));
	};

	UTEST_SCOPE("测试 HTTPS 网络请求。")
	{
		coro::IoContext io_context;
		net::HttpHeaders headers;
		headers.insert("user-agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/150.0.0.0 Safari/537.36 Edg/150.0.0.0");
		auto task1 = internet(&io_context, net::uri("https://www.bilibili.com"), headers);
		auto task2 = internet(&io_context, net::uri("https://www.baidu.com"), headers);
		io_context.add(task1.coroutine());
		io_context.add(task2.coroutine());

		io_context.run();
	};

	return UTEST_COMPLETE();
}
