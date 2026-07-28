#include <ayr/net/http.hpp>
#include <ayr/base/utest.hpp>

using namespace ayr;

void test_uri()
{
	net::Uri full = net::uri("https://example.com:8443/api/v1?q=openai&page=2#top"as);
	AYR_TEST_EXPECT_EQ(full.scheme(), "https"as);
	AYR_TEST_EXPECT_EQ(full.host(), "example.com"as);
	AYR_TEST_EXPECT_EQ(full.port(), "8443"as);
	AYR_TEST_EXPECT_EQ(full.path(), "/api/v1"as);
	AYR_TEST_EXPECT_EQ(full.queries().get("q"as), "openai"as);
	AYR_TEST_EXPECT_EQ(full.queries().get("page"as), "2"as);
	AYR_TEST_EXPECT_EQ(full.fragment(), "top"as);

	net::Uri fragment_only = net::uri("https://example.com/path#frag"as);
	AYR_TEST_EXPECT_EQ(fragment_only.fragment(), "frag"as);

	net::Uri relative = net::uri("/search?q=cpp#result"as);
	AYR_TEST_EXPECT_EQ(relative.host(), ""as);
	AYR_TEST_EXPECT_EQ(relative.path(), "/search"as);
	AYR_TEST_EXPECT_EQ(relative.queries().get("q"as), "cpp"as);
	AYR_TEST_EXPECT_EQ(relative.fragment(), "result"as);

	net::Uri bare_query = net::uri("https://example.com/path?flag=1&empty=2#frag"as);
	AYR_TEST_EXPECT_EQ(bare_query.queries().get("flag"as), "1"as);
	AYR_TEST_EXPECT_EQ(bare_query.queries().get("empty"as), "2"as);
}

void test_request()
{
	net::HttpRequest request(
		"POST"as,
		net::uri("https://example.com:8443/search?q=cpp"as),
		"HTTP/1.1"as
	);
	request.set_body("你好");

	AYR_TEST_EXPECT_EQ(request.path(), "/search?q=cpp"as);
	AYR_TEST_EXPECT_EQ(request.headers.get("host"as), "example.com"as);
	AYR_TEST_EXPECT_EQ(request.headers.get("CONTENT-LENGTH"as), "6"as);
	AYR_TEST_EXPECT_EQ(request.body, "你好"as.encode());

	Buffer serialized;
	serialized << request;
	CString request_bytes = vstr(serialized.peek(), serialized.readable_size());
	AYR_TEST_EXPECT(request_bytes.contains("Content-Length: 6\r\n"));
	AYR_TEST_EXPECT(request_bytes.endswith("你好"));

	request.set_body("");
	AYR_TEST_EXPECT(request.body.empty());
	AYR_TEST_EXPECT(!request.headers.contains("content-length"as));
	AYR_TEST_EXPECT_AYR_ERROR(request.add_header("Bad Header"as, "value"as));
	AYR_TEST_EXPECT_AYR_ERROR(request.add_header("X-Test"as, "one\r\ntwo"as));

	net::Uri manual_uri;
	manual_uri.scheme("HTTP"as);
	manual_uri.host("example.com"as);
	net::HttpRequest manual_request("GET"as, manual_uri, "HTTP/1.1"as);
	AYR_TEST_EXPECT_EQ(manual_request.uri().scheme(), "http"as);
	tlog(manual_request.path());
	AYR_TEST_EXPECT_EQ(manual_request.path(), "/"as);
}

void test_fixed_length_response()
{
	net::HttpResponse response;
	net::ResponseParser parser("GET"as);
	Buffer buffer;
	buffer <<
		"HTTP/1.1 200 OK\r\n"
		"content-length: 5\r\n"
		"Set-Cookie: first=1\r\n"
		"set-cookie: second=2\r\n"
		"X-Test: one\r\n\r\nhe";

	AYR_TEST_EXPECT(!parser(response, buffer));
	buffer << "lloNEXT";
	AYR_TEST_EXPECT(parser(response, buffer));
	AYR_TEST_EXPECT_EQ(response.status_code, 200);
	AYR_TEST_EXPECT_EQ(response.headers.get("Content-Length"as), "5"as);
	AYR_TEST_EXPECT_EQ(response.body, "hello");
	AYR_TEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), "NEXT");
}

void test_chunked_response()
{
	net::HttpResponse response;
	net::ResponseParser parser("GET"as);
	Buffer buffer;
	buffer << "HTTP/1.1 200 OK\r\nTransfer-Encoding: Chunked\r\n\r\n4\r\nWi";

	AYR_TEST_EXPECT(!parser(response, buffer));
	AYR_TEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), "Wi");

	buffer << "ki\r\n5;name=value\r\npedia\r\n0\r\nX-Trailer: done\r\n\r\n";
	AYR_TEST_EXPECT(parser(response, buffer));
	AYR_TEST_EXPECT_EQ(response.body, "Wikipedia");
	AYR_TEST_EXPECT_EQ(response.headers.get("x-trailer"as), "done"as);
	AYR_TEST_EXPECT(buffer.readable_size() == 0);
}

void test_close_delimited_and_no_body_responses()
{
	net::HttpResponse close_response;
	net::ResponseParser close_parser("GET"as);
	Buffer close_buffer;
	close_buffer << "HTTP/1.1 200 OK\r\nConnection: close\r\n\r\nclose body";

	AYR_TEST_EXPECT(!close_parser(close_response, close_buffer));
	AYR_TEST_EXPECT(close_parser(close_response, close_buffer, true));
	AYR_TEST_EXPECT_EQ(close_response.body, "close body");

	net::HttpResponse no_body_response;
	net::ResponseParser no_body_parser("GET"as);
	Buffer no_body_buffer;
	no_body_buffer << "HTTP/1.1 204 No Content\r\nDate: now\r\n\r\n";
	AYR_TEST_EXPECT(no_body_parser(no_body_response, no_body_buffer));
	AYR_TEST_EXPECT(no_body_response.body.empty());

	net::HttpResponse head_response;
	net::ResponseParser head_parser("HEAD"as);
	Buffer head_buffer;
	head_buffer << "HTTP/1.1 200 OK\r\nContent-Length: 123\r\n\r\n";
	AYR_TEST_EXPECT(head_parser(head_response, head_buffer));
	AYR_TEST_EXPECT(head_response.body.empty());
}

void test_interim_and_invalid_responses()
{
	net::HttpResponse interim_response;
	net::ResponseParser interim_parser("POST"as);
	Buffer interim_buffer;
	interim_buffer <<
		"HTTP/1.1 101 Continue\r\n\r\n"
		"HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK";
	AYR_TEST_EXPECT(interim_parser(interim_response, interim_buffer));
	AYR_TEST_EXPECT_EQ(interim_response.status_code, 200);
	AYR_TEST_EXPECT_EQ(interim_response.body, "OK");

	net::HttpResponse truncated_response;
	net::ResponseParser truncated_parser("GET"as);
	Buffer truncated_buffer;
	truncated_buffer << "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nabc";
	AYR_TEST_EXPECT(!truncated_parser(truncated_response, truncated_buffer));
	AYR_TEST_EXPECT_AYR_ERROR(truncated_parser(truncated_response, truncated_buffer, true));

	net::HttpResponse ambiguous_response;
	net::ResponseParser ambiguous_parser("GET"as);
	Buffer ambiguous_buffer;
	ambiguous_buffer <<
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: 1\r\n"
		"Transfer-Encoding: chunked\r\n\r\n";
	AYR_TEST_EXPECT_AYR_ERROR(ambiguous_parser(ambiguous_response, ambiguous_buffer));

	net::HttpResponse invalid_chunk_response;
	net::ResponseParser invalid_chunk_parser("GET"as);
	Buffer invalid_chunk_buffer;
	invalid_chunk_buffer <<
		"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n"
		"3\r\nabcXX";
	AYR_TEST_EXPECT_AYR_ERROR(invalid_chunk_parser(
		invalid_chunk_response,
		invalid_chunk_buffer
	));
}

coro::Task<void> internet(coro::IoContext* io_context, net::Uri uri, net::HttpHeaders headers)
{
	auto resp = co_await net::get(io_context, uri, headers);
	AYR_TEST_EXPECT_EQ(resp.status_code, 200);
}


void test_internet()
{
	coro::IoContext io_context;
	net::HttpHeaders headers;
	headers.insert("user-agent"as, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/150.0.0.0 Safari/537.36 Edg/150.0.0.0"as);
	auto task1 = internet(&io_context, net::uri("https://www.bilibili.com"as), headers);
	auto task2 = internet(&io_context, net::uri("https://www.baidu.com"as), headers);
	io_context.add(task1.coroutine());
	io_context.add(task2.coroutine());

	io_context.run();
}

int main()
{
	static_assert(!std::is_copy_constructible_v<net::Session>);
	static_assert(std::is_move_constructible_v<net::Session>);

	test_uri();
	test_request();
	test_fixed_length_response();
	test_chunked_response();
	test_close_delimited_and_no_body_responses();
	test_interim_and_invalid_responses();
	test_internet();
}
