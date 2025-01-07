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
#include <ayr/json.hpp>

using namespace ayr;

void json_test()
{
	tlog(json::parse("123"));
	tlog(json::parse("true"));
	tlog(json::parse("false"));
	tlog(json::parse("null"));
	tlog(json::parse(R"("abc")"));
	tlog(json::parse("[]"));
	tlog(json::parse("[1, 2, 3]"));
	tlog(json::parse("{}"));
	tlog(json::parse(R"({"a": 1})"));
	tlog(json::parse(R"({"a": 1, "b": 2})"));
	tlog(json::parse(R"({"a": 1, "b": [2, 3]})"));
	tlog(json::parse(R"({"a": 1, "b": {"c": 2}})"));
	tlog(json::parse(R"({"a": 1, "b": {"c": 2, "d": [3, 4]}})"));

	Atring json_str = R"({"array": [1, 2, "3", 4, 5.6, ["a", "b", "c"], {"d": 1, "e": 2, "f": 3}]})";
	tlog(json_str);
	json::Json json_obj = json::parse(json_str);
	tlog(json_obj);
	for (auto& item : json_obj["array"as].transform<json::JsonType::JsonArray>())
		tlog(item);
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
pass time: 677.38 ms
test_vs
pass time: 3003.89 ms
*/
```

## Chain&BiChain单链表双链表
添加速度与std::list相当，随机删除速度快std::list 6倍
```cpp
def chain_speed_test()
{
	Timer_ms timer;
	Chain<CString> chain;
	BiChain<CString> bichain;
	std::list<CString> stdlist;
	std::forward_list<CString> stdflist;

	int N = 1e6;

	timer.into();
	for (int i = 0; i < N; i++)
		chain.append("hello");
	print("chain append time: ", timer.escape(), "ms");

	timer.into();
	for (int i = 0; i < N; i++)
		bichain.append("hello");
	print("bichain append time: ", timer.escape(), "ms");

	timer.into();
	for (int i = 0; i < N; i++)
		stdlist.push_back("hello");
	print("std::list append time: ", timer.escape(), "ms");

	timer.into();
	for (int i = 0; i < N; i++)
		stdflist.push_front("hello");
	print("std::forward_list append time: ", timer.escape(), "ms");

	timer.into();
	auto cnd = chain.at_node(1000);
	while (chain.size() > 1005)
		chain.pop_from(cnd->next_node(), 1, cnd);
	print("chain random pop time: ", timer.escape(), "ms");

	timer.into();
	auto bicnd = bichain.at_node(1000);
	while (bichain.size() > 1005)
		bichain.pop_from(bicnd->next_node(), 1);
	print("bichain random pop time: ", timer.escape(), "ms");

	timer.into();
	auto stdit = stdlist.begin();
	std::advance(stdit, 1000);
	while (stdlist.size() > 1005)
	{
		auto _0 = stdit++;
		auto _1 = stdit++;
		stdlist.erase(_1, stdit);
		stdit = _0;
	}

	print("std::list random pop time: ", timer.escape(), "ms");

}
/*
输出:
chain append time:  478 ms
bichain append time:  461 ms
std::list append time:  601 ms
std::forward_list append time:  559 ms
chain random pop time:  196 ms
bichain random pop time:  216 ms
std::list random pop time:  1213 ms
*/
```
## 协程
使用生成器函数
```cpp
#include "ayr/coro.hpp"

using namespace ayr;

coro::Generator<int> numbers(int n)
{
	for (int i = n; i; i--)
		co_yield i;
	co_return n + 1;
}

void generator_test()
{
	for (int i : numbers(10))
		print(i);
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
11
*/
```

## 协程打印hello world
```cpp
#include "ayr/coro.hpp"
#include "ayr/base/CString.hpp"

using namespace ayr;
using namespace coro;

Task<CString> hello()
{
	print("into hello, wait for 2s");
	co_await coro::Sleep(2s);
	print("hello");
	co_return "hello";
}

Task<int> world()
{
	print("into world, wait for 1s");
	co_await coro::Sleep(1s);
	print("world");
	co_return 114;
}

coro::Task<void> when_all_hello_world()
{
	auto [a, b] = co_await coro::when_all(hello(), world());
	print(a, b);
}

void when_all_test()
{
	asyncio.add(when_all_hello_world());
	asyncio.run();
}
/*
输出:
into world, wait for 1s
into world, wait for 1s
world
hello
hello 114
*/
```

## 协程睡眠排序
```cpp
#include <ayr/coro.hpp>

using namespace ayr::coro;

void sleep_sort_test()
{
	auto async_num = [](int num) -> Task<void>
		{
			co_await coro::Sleep(num * 1s);
			print(num);
			co_return;
		};

	asyncio.add(async_num(3));
	asyncio.add(async_num(2));
	asyncio.add(async_num(1));
	asyncio.add(async_num(5));
	asyncio.add(async_num(4));
	asyncio.run();
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
```cpp
#include <ayr/net.hpp>

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