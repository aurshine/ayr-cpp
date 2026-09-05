#include <atomic>
#include <chrono>
#include <thread>
#include <type_traits>

#include <ayr/net/http.hpp>
#include <ayr/base/utest.hpp>

using namespace ayr;

namespace
{
	constexpr const char* HOST = "127.0.0.1";
	constexpr int PORT = 7783;

	coro::Task<CString> read_request(net::Socket& socket, const CString& terminator)
	{
		Buffer request;
		while (true)
		{
			CString bytes = vstr(request.peek(), request.readable_size());
			if (bytes.endswith(terminator))
				co_return dstr(request.peek(), request.readable_size());

			request.adjust_util(1024);
			net::IoResult result = co_await socket.read(request);
			if (!result.ok())
				RuntimeError(result.error);
			if (result.bytes == 0)
				RuntimeError("Client closed before sending a complete HTTP request.");
		}
	}

	coro::Task<void> write_response(net::Socket& socket, const CString& response)
	{
		Buffer buffer;
		buffer << response;
		net::IoResult result = co_await socket.write(buffer);
		if (!result.ok())
			RuntimeError(result.error);
	}

	coro::Task<void> http_server(coro::IoContext* io_context, std::atomic<bool>* ready)
	{
		net::Acceptor acceptor(HOST, PORT, io_context);
		acceptor.listen();
		*ready = true;

		{
			auto socket = co_await acceptor.accept();
			CString request = co_await read_request(socket, "\r\n\r\n");
			UTEST_EXPECT(request.contains("GET /fixed?value=1 HTTP/1.1\r\n"));
			UTEST_EXPECT(request.contains("Host: 127.0.0.1\r\n"));
			co_await write_response(socket, cstr(
				"HTTP/1.1 200 OK\r\n"
				"Content-Type: text/plain\r\n"
				"Content-Length: 5\r\n\r\n"
				"hello"
			));
		}

		{
			auto socket = co_await acceptor.accept();
			CString request = co_await read_request(socket, "ping");
			UTEST_EXPECT(request.contains("POST /events HTTP/1.1\r\n"));
			UTEST_EXPECT(request.contains("Content-Length: 4\r\n"));
			UTEST_EXPECT(request.contains("Content-Type: text/plain; charset="));
			co_await write_response(socket, cstr(
				"HTTP/1.1 200 OK\r\n"
				"Content-Type: text/event-stream\r\n"
				"Transfer-Encoding: chunked\r\n\r\n"
				"D\r\ndata: first\n\n\r\n"
				"E;event=message\r\ndata: second\n\n\r\n"
				"0\r\nX-Stream-End: yes\r\n\r\n"
			));
		}

		{
			auto socket = co_await acceptor.accept();
			CString request = co_await read_request(socket, "\r\n\r\n");
			UTEST_EXPECT(request.contains("HEAD /metadata HTTP/1.1\r\n"));
			co_await write_response(socket, cstr(
				"HTTP/1.1 204 No Content\r\n"
				"Content-Length: 100\r\n\r\n"
			));
		}
	}

	coro::Task<void> http_client(coro::IoContext* io_context)
	{
		net::Session session;
		CString origin = cstr(ayr::format("http://{}:{}", HOST, PORT));

		{
			auto response = co_await session.get(io_context, net::uri(origin + "/fixed?value=1"));
			UTEST_EXPECT_EQ(response.status_code, 200);
			UTEST_EXPECT_EQ(response.status_message, "OK");
			UTEST_EXPECT_EQ(response.headers.get("content-type"), "text/plain");
			UTEST_EXPECT_EQ(response.body, "hello");
		}

		{
			auto response = co_await session.post(
				io_context, net::uri(origin + "/events"), {}, "ping", true
			);
			UTEST_EXPECT_EQ(response.status_code, 200);
			UTEST_EXPECT_EQ(response.headers.get("Content-Type"), "text/event-stream");
			UTEST_EXPECT(response.body.empty());

			Buffer events;
			int chunks = 0;
			for (auto& next : response.stream())
			{
				CString chunk = co_await next;
				if (!chunk.empty())
				{
					events << chunk;
					++chunks;
				}
			}

			UTEST_EXPECT_EQ(chunks, 2);
			UTEST_EXPECT_EQ(from_buffer(std::move(events)), "data: first\n\ndata: second\n\n");
			UTEST_EXPECT_EQ(response.trailers().get("x-stream-end"), "yes");
		}

		{
			auto response = co_await session.request(
				io_context, "HEAD", net::uri(origin + "/metadata")
			);
			UTEST_EXPECT_EQ(response.status_code, 204);
			UTEST_EXPECT_EQ(response.headers.get("Content-Length"), "100");
			UTEST_EXPECT(response.body.empty());
		}
	}

	void test_session_and_streaming()
	{
		std::atomic<bool> ready = false;
		std::thread server([&] {
			coro::IoContext io_context;
			io_context.run(http_server(&io_context, &ready));
		});

		while (!ready.load())
			std::this_thread::sleep_for(std::chrono::milliseconds(1));

		coro::IoContext io_context;
		io_context.run(http_client(&io_context));
		server.join();
	}
}

int main()
{
	static_assert(!std::is_copy_constructible_v<net::Session>);
	static_assert(std::is_move_constructible_v<net::Session>);
	static_assert(!std::is_copy_constructible_v<net::HttpResponse>);
	static_assert(std::is_move_constructible_v<net::HttpResponse>);

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

		net::Uri relative = net::uri("/search?q=cpp#result");
		UTEST_EXPECT_EQ(relative.host(), "");
		UTEST_EXPECT_EQ(relative.path(), "/search");
		UTEST_EXPECT_EQ(relative.queries().get("q"), "cpp");
		UTEST_EXPECT_EQ(relative.fragment(), "result");
	};

	UTEST_SCOPE("测试 HTTP 请求构造、序列化和头部校验。")
	{
		net::HttpRequest request(
			"POST", net::uri("https://example.com:8443/search?q=cpp"), "HTTP/1.1"
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
	};

	UTEST_SCOPE("测试 Session 的普通响应、SSE 流式响应和无正文响应。")
	{
		test_session_and_streaming();
	};

	return UTEST_COMPLETE();
}
