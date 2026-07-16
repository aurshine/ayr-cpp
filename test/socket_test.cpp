#include <atomic>
#include <thread>

#include <ayr/net/Socket.hpp>

#include <ayr/base/utest.hpp>

using namespace ayr;

const char* HOST = "127.0.0.1";
constexpr int PORT = 7777;

coro::Task<void> client_main(coro::IoContext* io_context, std::atomic<bool>* client_verified)
{
	// 测试客户端能连接本地服务端、写入消息并读回 echo。
	auto client_fd = co_await net::open_connect(HOST, PORT, io_context);
	Buffer request;
	request << "Hello, world!";
	co_await client_fd.write(request);

	Buffer response(1024);
	co_await client_fd.read(response);
	AYR_TEST_EXPECT_EQ(vstr(response.peek(), response.readable_size()), "Hello, world!");
	*client_verified = true;
}

void client_thread(std::atomic<bool>* client_verified)
{
	coro::IoContext io_context;
	io_context.run(client_main(&io_context, client_verified));
}

coro::Task<void> server_main(coro::IoContext* io_context, std::atomic<bool>* server_verified)
{
	// 测试服务端能 accept 本地连接、读取完整消息并原样写回。
	net::Acceptor acceptor(HOST, PORT, io_context);
	acceptor.listen();
	auto fd = co_await acceptor.accept();

	Buffer buffer(1024);
	co_await fd.read(buffer);
	AYR_TEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), "Hello, world!");
	co_await fd.write(buffer);
	*server_verified = true;
}

void server_thread(std::atomic<bool>* server_verified)
{
	coro::IoContext io_context;
	io_context.run(server_main(&io_context, server_verified));
}

int main()
{
	// 测试本地 TCP echo 往返；这是集成测试，依赖 127.0.0.1:7777 端口可用。
	std::atomic<bool> client_verified = false;
	std::atomic<bool> server_verified = false;
	std::thread server_thread_obj(server_thread, &server_verified);
	std::this_thread::sleep_for(std::chrono::milliseconds(200));
	client_thread(&client_verified);
	server_thread_obj.join();

	AYR_TEST_EXPECT(client_verified.load());
	AYR_TEST_EXPECT(server_verified.load());
	return 0;
}
