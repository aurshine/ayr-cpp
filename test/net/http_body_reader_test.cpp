#include <atomic>
#include <chrono>
#include <thread>
#include <ayr/base/Array.hpp>

#include <ayr/net/http.hpp>
#include <ayr/base/utest.hpp>

using namespace ayr;

namespace
{
	const CString HOST = "127.0.0.1";
	constexpr int PORT = 7782;
	std::atomic<bool> line_received = false;

	struct LineCase
	{
		CString first;
		CString rest;
		Array<CString> lines;
	};

	const CString FIXED_HEAD = "HTTP/1.1 200 OK\r\nContent-Length: ";
	const CString CHUNK_HEAD = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n";
	const CString LONG_LINE = (CString("x") * 9000) + "\n";
	const Array<LineCase> LINE_CASES = {
		{ FIXED_HEAD + "15\r\n\r\none\n\r\ntwo\r\ntail\nignored", "", { "one\n", "\r\n", "two\r\n", "tail" } },
		{ FIXED_HEAD + "13\r\n\r\none\npar\r", "\ntail", { "one\n", "par\r\n", "tail" } },
		{ FIXED_HEAD + "4\r\n\r\none\n", "", { "one\n" } },
		{ FIXED_HEAD + "0\r\n\r\nignored\n", "", Array<CString>(0) },
		{ FIXED_HEAD + fmt("{}", LONG_LINE.size() + 4) + "\r\n\r\n" + LONG_LINE + "tail", "", { LONG_LINE, "tail" } },
		{ CHUNK_HEAD + "12\r\none\npar\r", "\ntail\nlast\r\n0\r\n\r\n", { "one\n", "par\r\n", "tail\n", "last" } },
		{ CHUNK_HEAD + "4\r\none\n", "\r\n4\r\ntail\r\n0\r\n\r\n", { "one\n", "tail" } },
		{ CHUNK_HEAD + "2\r\non\r\n2\r\ne\n\r", "\n3\r\nab\r\r\n3\r\n\ncd\r\n1\r\n\n\r\n4\r\ntail\r\n0\r\nX-End: yes\r\n\r\n", { "one\n", "ab\r\n", "cd\n", "tail" } },
		{ CHUNK_HEAD + "2329\r\n" + LONG_LINE + "\r\n0\r\n\r\n", "", { LONG_LINE } },
		{ CHUNK_HEAD + "0\r\n\r\n", "", Array<CString>(0) }
	};
	const Array<CString> INVALID_CASES = {
		FIXED_HEAD + "8\r\n\r\none\nxy",
		CHUNK_HEAD + "8\r\none\nxy",
		CHUNK_HEAD + "4\r\none\nxx",
		CHUNK_HEAD + "4\r\none\n\r",
		CHUNK_HEAD + "3\r\nabc\r\n"
	};

	coro::Task<void> write_response(net::Socket& socket, CString response)
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
			co_await write_response(socket, cstr(
				"HTTP/1.1 200 OK\r\n"
				"Transfer-Encoding: chunked\r\n\r\n"
				"5\r\nhello\r\n"
				"6\r\n world\r\n"
				"0\r\nX-End: yes\r\n\r\n"
			));
		}

		{
			auto socket = co_await acceptor.accept();
			co_await write_response(socket, cstr(
				"HTTP/1.1 200 OK\r\nContent-Length: 100\r\n\r\n"
			));
		}

		{
			auto socket = co_await acceptor.accept();
			co_await write_response(socket, cstr(
				"HTTP/1.1 100 Continue\r\n\r\n"
				"HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK"
			));
		}

		for (const auto& test_case : LINE_CASES)
		{
			auto socket = co_await acceptor.accept();
			line_received = false;
			co_await write_response(socket, test_case.first);
			if (!test_case.rest.empty())
			{
				// 客户端读到首行后才发送剩余正文，验证不会等待整个 size。
				while (!line_received.load())
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				co_await write_response(socket, test_case.rest);
			}
		}
		for (const auto& response : INVALID_CASES)
		{
			auto socket = co_await acceptor.accept();
			co_await write_response(socket, response);
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

		for (const auto& test_case : LINE_CASES)
		{
			auto socket = co_await net::open_connect(HOST, PORT, io_context);
			net::BodyReader reader(std::move(socket), vstr("GET"));
			UTEST_EXPECT_EQ((co_await reader.parse_head()).status_code, 200);
			for (std::size_t i = 0; i < test_case.lines.size(); ++i)
			{
				UTEST_EXPECT(!reader.done());
				UTEST_EXPECT_EQ(co_await reader.read(), dstr(test_case.lines[i]));
				if (i == 0 && !test_case.rest.empty())
				{
					UTEST_EXPECT(!reader.done());
					line_received = true;
				}
			}
			if (!reader.done())
				UTEST_EXPECT((co_await reader.read()).empty());
			UTEST_EXPECT(reader.done());
			UTEST_EXPECT((co_await reader.read()).empty());
		}
		for (const auto& response : INVALID_CASES)
		{
			auto socket = co_await net::open_connect(HOST, PORT, io_context);
			net::BodyReader reader(std::move(socket), vstr("GET"));
			bool rejected = false;
			try
			{
				while (!reader.done())
					co_await reader.read();
			}
			catch (const AyrError& error)
			{
				rejected = error.is(vstr("ValueError"));
			}
			UTEST_EXPECT(rejected);
			UTEST_EXPECT(!reader.done());
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
