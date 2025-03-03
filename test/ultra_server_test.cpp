#include "ayr/net.hpp"

#if defined(AYR_LINUX)
class TServer : public ayr::UltraTcpServer<TServer>
{
	using super = ayr::UltraTcpServer<TServer>;
public:
	TServer(int port) : super(port, 0, 10000) {}

	// 客户端连接到服务器后调用的回调函数
	void on_connected(const Socket& client)
	{
		++count_in;
		ayr::print("conected: ", client);
	}

	// 客户端断开连接时调用的回调函数
	void on_disconnected(const Socket& client)
	{
		++ count_out;
		ayr::print("disconnected: ", client);
	}

	// 读事件产生时的回调函数, 返回读取的数据
	void on_reading(const Socket& client)
	{
		print("from: ", client, "recv: ", client.recv(1024));
	}

	// 写事件产生时的回调函数, 用于发送数据
	void on_writing(const Socket& client) {}

	void on_timeout()
	{
		print("count_in: ", count_in, "count_out: ", count_out);
		throw std::runtime_error("timeout");
	}

	int count_in = 0, count_out = 0;
};

void ultra_server_test()
{
	TServer server(5555);
	print("server start");
	server.run();
}
#endif