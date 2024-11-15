# c++开发库(开发中)

##  关于
这个库志于使用c++20实现一些易用简洁的组件用于日常工具，包括但不限于:
1. 容器
2. json解析
3. 多线程并发
4. 协程
5. 字符编码
6. 文件操作
7. 网络通讯

## json 格式解析
在json/parse.hpp中实现对json格式的字符串解析为一个json对象
```cpp
#include <ayr/json/parse.hpp>

using namespace ayr;


int main()
{
	Atring json_str = R"({"array": [1, 2, "3", 4, 5.6, ["a", "b", "c"], {"d": 1, "e": 2, "f": 3}]})";
	json::Json json_obj = json::parse(json_str);

	for (auto& item : json_obj["array"as].transform<json::JsonType::JsonArray>())
		print(item);

	return 0;
}
```

## 动态数组 DynArray
对于拷贝昂贵的类型，DynArray远快于vector
```cpp
#include <vector>
#include <string>
#include <ayr/Array.hpp>
#include <ayr/DynArray.hpp>
#include <ayr/timer.hpp>

ayr::DynArray<std::string> das;
std::vector<std::string> vs;

constexpr int N = 1e6;

void test_das()
{
	print("test_das");
	for (int i = 0; i < N; ++i)
	{
		das.append("hello");
	}
}

void test_vs()
{
	print("test_vs");
	for (int i = 0; i < N; ++i)
	{
		vs.push_back("hello");
	}
}

int main()
{
	auto tm = ayr::Timer("ms");

	tm(test_das);
	tm(test_vs);

	return 0;
}

/*
输出:
test_das
pass time: 723.38 ms
test_vs
pass time: 2700.41 ms
*/
```

## 协程
使用生成器函数
```cpp
#include <ayr/coro/coro.hpp>

using namespace ayr;

coro::Generator<int> dfs(int n)
{
	while (n > 0)
		co_yield n--;
}


int main()
{
	for (int i : dfs(10))
		print(i);

	return 0;
}

/* 
输出: 
10
9
8
7
6
5
4
3
2
1
*/
```

## 协程打印hello world
```cpp
#include <ayr/coro/coro.hpp>


using namespace ayr;
using namespace std::chrono_literals;

coro::Task<int> hello()
{
	print("hello");
	co_await coro::Sleep(2s);
	print("hello * 2");
	co_return 114;
}

coro::Task<int> world()
{
	print("world");
	co_await coro::Sleep(1s);
	print("world * 2");
	co_return 514;
}


int main()
{
	auto& hello_promise = coro::CoroLoop::add(hello());
	print("add coroutine hello");
	auto& world_promise = coro::CoroLoop::add(world());
	print("add coroutine world");
	coro::CoroLoop::run();

	print("hello result:", hello_promise.result(), "world result:", world_promise.result());
}
/*
输出:
add coroutine hello
add coroutine world
hello
world
world * 2
hello * 2
hello result: 114 world result: 514
*/
```

## 协程睡眠排序
```
#include <ayr/coro/coro.hpp>

using namespace ayr;
using namespace std::chrono_literals;

coro::Task<void> async_num(int num)
{
	co_await coro::Sleep(num * 1s);
	print(num);
	co_return;
}

int main()
{
	coro::CoroLoop::add(async_num(3));
	coro::CoroLoop::add(async_num(2));
	coro::CoroLoop::add(async_num(1));
	coro::CoroLoop::add(async_num(5));
	coro::CoroLoop::add(async_num(4));
	coro::CoroLoop::run();
	return 0;
}

/*
输出:
1
2
3
4
5
*/
```

## tcp回声服务器
```
#include <ayr/socket/tcp.hpp>

constexpr int PORT = 14514;

void tcp_echo_server()
{
	ayr::TcpServer server(nullptr, PORT);
	Socket& client = server.accept();
	while (true)
	{
		CString data = server.recv(0);
		if (!data) break;
		server.send(0, data, data.size());
	}
	print("server exit");
}

void tcp_echo_client()
{
	ayr::TcpClient client("127.0.0.1", PORT);

	while (true)
	{
		char data[1024];
		print.setend(" ");
		print("input:");
		print.setend("\n");
		std::cin >> data;
		if (data[0] == 'q' || data[0] == 'Q')
			break;

		client.send(data, strlen(data));
		ayr::print("server response:", client.recv());
	}
	print("client exit");
}

/*
输出:
client input message: hello
server recv message: hello

client input message: hi
server recv message: hi

client input message: Q
*/
```

## udp回声服务器
```cpp
void udp_echo_server_test()
{
	ayr::UdpServer server(nullptr, PORT);
	while (true)
	{
		auto [data, client_addr] = server.recv();
		print(data, client_addr);
		if (!data) break;
		server.send(data, data.size(), client_addr);
	}
	print("udp server exit");
}

void udp_echo_client_test()
{
	ayr::UdpClient client("127.0.0.1", PORT);

	while (true)
	{
		char data[1024];
		print.setend(" ");
		print("input:");
		print.setend("\n");
		std::cin >> data;
		if (data[0] == 'q' || data[0] == 'Q')
			break;

		client.send(data, strlen(data));
		ayr::print("server response:", client.recv().first);
	}
	print("udp client exit");
}

void udp_echo_test()
{
	std::thread server(udp_echo_server_test);
	std::this_thread::sleep_for(1s); // 等待服务端启动
	std::thread client(udp_echo_client_test);

	server.join();
	client.join();
}
```