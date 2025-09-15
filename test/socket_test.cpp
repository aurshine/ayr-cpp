#include <thread>

#include <ayr/net/Socket.hpp>

const char* HOST = "127.0.0.1";

constexpr int PORT = 7777;

ayr::coro::Task<void> client_main(ayr::coro::IoContext* io_context)
{
	auto client_fd = co_await ayr::net::open_connect(HOST, PORT, io_context);
	ayr::print("client connected to server: ", client_fd);
	co_await client_fd.write("Hello, world!");
	ayr::print("client sent: Hello, world!");
	ayr::Buffer buffer(1024);
	co_await client_fd.read(buffer);
	ayr::print("client received: ", ayr::vstr(buffer.peek(), buffer.readable_size()));
}

void client_thread()
{
	ayr::coro::IoContext io_context;
	ayr::print("client start");
	io_context.run(client_main(&io_context));
	ayr::print("client end");
}

ayr::coro::Task<void> server_main(ayr::coro::IoContext* io_context)
{
	ayr::net::Acceptor acceptor(HOST, PORT, io_context);
	ayr::print("server bind", HOST, ":", PORT);
	acceptor.listen();
	ayr::print("server listening");
	auto fd = co_await acceptor.accept();
	ayr::print("server accepted client: ", fd);
	ayr::Buffer buffer(1024);
	co_await fd.read(buffer);
	ayr::print("server received: ", ayr::vstr(buffer.peek(), buffer.readable_size()), buffer.readable_size(), "bytes");
	co_await fd.write(buffer);
}

void server_thread()
{
	ayr::coro::IoContext io_context;
	ayr::print("server start");
	io_context.run(server_main(&io_context));
	ayr::print("server end");
}

int main()
{
	std::thread server_thread_obj(server_thread);
	std::this_thread::sleep_for(std::chrono::seconds(1));
	client_thread();
	server_thread_obj.join();
	return 0;
}