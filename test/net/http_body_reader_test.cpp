#include <atomic>
#include <chrono>
#include <thread>

#include <ayr/net/http.hpp>
#include <ayr/base/utest.hpp>

using namespace ayr;

namespace
{
	constexpr const char* HOST = "127.0.0.1";
	constexpr int PORT = 7782;

	coro::Task<void> write_response(net::Socket socket, CString response)
	{
		Buffer buffer;
		buffer << response;
		net::IoResult result = co_await socket.write(buffer);
		UTEST_EXPECT(result.ok());
	}

	coro::Task<void> server_main(coro::IoContext* io_context, std::atomic<bool>* ready)
	{
		net::Acceptor acceptor(HOST, PORT, io_context);
		acceptor.listen();
		*ready = true;

		{
			auto socket = co_await acceptor.accept();
			co_await write_response(std::move(socket), cstr(
				"HTTP/1.1 200 OK\r\n"
				"Transfer-Encoding: chunked\r\n\r\n"
				"5\r\nhello\r\n"
				"6\r\n world\r\n"
				"0\r\nX-End: yes\r\n\r\n"
			));
		}

		{
			auto socket = co_await acceptor.accept();
			co_await write_response(std::move(socket), cstr(
				"HTTP/1.1 200 OK\r\nContent-Length: 100\r\n\r\n"
			));
		}

		{
			auto socket = co_await acceptor.accept();
			co_await write_response(std::move(socket), cstr(
				"HTTP/1.1 100 Continue\r\n\r\n"
				"HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK"
			));
		}
	}

	coro::Task<void> client_main(coro::IoContext* io_context)
	{
		{
			auto socket = co_await net::open_connect(HOST, PORT, io_context);
			net::BodyReader reader(std::move(socket), vstr("GET"));
			net::HttpResponseHead head = co_await reader.parse_head();
			UTEST_EXPECT_EQ(head.status_code, 200);

			// parse_head 可能已经预读了响应体；移动后必须保留这些字节。
			net::BodyReader moved_reader(std::move(reader));
			Buffer body;
			while (!moved_reader.done())
				body << co_await moved_reader.read();
			UTEST_EXPECT_EQ(from_buffer(std::move(body)), "hello world");
			UTEST_EXPECT_EQ(moved_reader.trailers().get("X-End"), "yes");
		}

		{
			auto socket = co_await net::open_connect(HOST, PORT, io_context);
			net::BodyReader reader(std::move(socket), vstr("HEAD"));
			net::HttpResponseHead head = co_await reader.parse_head();
			UTEST_EXPECT_EQ(head.status_code, 200);
			UTEST_EXPECT_EQ(head.headers.get("Content-Length"), "100");
			UTEST_EXPECT((co_await reader.read()).empty());
			UTEST_EXPECT(reader.done());
		}

		{
			auto socket = co_await net::open_connect(HOST, PORT, io_context);
			net::BodyReader reader(std::move(socket), vstr("POST"));
			net::HttpResponseHead head = co_await reader.parse_head();
			UTEST_EXPECT_EQ(head.status_code, 200);
			UTEST_EXPECT_EQ(co_await reader.read(), "OK");
		}
	}
}

int main()
{
	std::atomic<bool> ready = false;
	std::thread server([&] {
		coro::IoContext io_context;
		io_context.run(server_main(&io_context, &ready));
	});

	while (!ready.load())
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

	coro::IoContext io_context;
	io_context.run(client_main(&io_context));
	server.join();
	return UTEST_COMPLETE();
}
