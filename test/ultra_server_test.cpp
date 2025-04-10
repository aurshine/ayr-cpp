#include "ayr/net.hpp"
#include <atomic>

using namespace ayr;

#if defined(AYR_LINUX)
class TServer : public ayr::UltraTcpServer<TServer>
{
	using super = ayr::UltraTcpServer<TServer>;
public:
	TServer(const CString& ip, int port, int num_thread, int timeout_s) : super(ip, port, num_thread, timeout_s) {}

	// 客户端连接到服务器后调用的回调函数
	void on_connected(const Socket& client)
	{
		++count_in;
		ayr::print("conected: ", client);
	}

	// 客户端断开连接时调用的回调函数
	void on_disconnected(const Socket& client)
	{
		++count_out;
		ayr::print("disconnected: ", client);
	}

	// 读事件产生时的回调函数, 返回读取的数据
	void on_reading(const Socket& client)
	{
		print("from: ", client, "recv: ", client.recv(1024));
		client.send("hello, world!");
	}

	// 写事件产生时的回调函数, 用于发送数据
	void on_writing(const Socket& client) {}

	void on_timeout()
	{
		print("count_in: ", count_in.load(), "count_out: ", count_out.load());
		stop();
		print("server stop");
	}

	std::atomic<int> count_in = 0, count_out = 0;
};

int main()
{
	TServer server("127.0.0.1", 5555, 0, 5000);
	print("server start");
	server.run();
}
#else
int main()
{
	ayr::print("not support this platform");
	return 0;
}
#endif