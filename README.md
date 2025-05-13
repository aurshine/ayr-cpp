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

## Atring字符串
提供比std::string更加高效，操作更加丰富的字符串
```cpp
#include <ayr/Atring.hpp>

using namespace ayr;

void atring_test()
{
	Atring str1 = "你好 世界";
	tlog(str1);
	tlog(str1.slice(1, 2));
	tlog(str1.replace("你"as, "你们"as));
	tlog(str1.split());
	tlog(str1.strip("你"));
	tlog(str1.startswith("你"));
	tlog(str1.endswith("你"));
	tlog("*"as.join(Array<const char*>{ "我", "爱", "你" }));
	tlog("adhaf{{这是内容}}ahfiaf{{dadf}}"as.match("{{", "}}"));
	tlog(*str1.encoding())
}
```

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
	for (auto& item : json_obj["array"as].as<json::JsonArray>())
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

## Chain双链表
添加速度与std::list相当，随机删除速度快std::list 6倍
```cpp
#include <list>
#include <forward_list>

#include "ayr/Chain.hpp"
#include "ayr/timer.hpp"

using namespace ayr;


int main()
{
	Timer_ms timer;
	Chain<CString> bichain;
	std::list<CString> stdlist;
	std::forward_list<CString> stdflist;

	int N = 1e6;

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
	auto bcnd = bichain.at_node(1000);
	while (bichain.size() > 1005)
		bichain.pop(bcnd->next());
	print("bichain random pop time: ", timer.escape(), "ms");

	timer.into();
	auto it = stdlist.begin();
	std::advance(it, 1000);
	while (stdlist.size() > 1005)
	{
		stdlist.erase(std::next(it, 1), std::next(it, 2));
	}

	print("std::list random pop time: ", timer.escape(), "ms");
	return 0;
}
/*
输出:
bichain append time:  387 ms
std::list append time:  473 ms
std::forward_list append time:  484 ms
bichain random pop time:  312 ms
std::list random pop time:  1095 ms
*/
```

## Dict字典
提供与unordered_map相同(略高)的效率, 更加丰富的功能
```cpp
#include <ayr/Dict.hpp>

using namespace ayr;

void dict_run_speed_test()
{
	Timer_ms t;
	Dict<std::string, int> d;
	std::vector<std::string> vs;
	constexpr int N = 1e6;

	for (int i = 0; i < N; i++)
		vs.push_back(std::to_string(i));

	t.into();
	for (int i = 0; i < N; i++)
		d.insert(vs[i], i);
	print("Dict insert time: ", t.escape(), "ms");
	t.into();
	for (int i = 0; i < N; i++)
		d[vs[i]];
	print("Dict query time: ", t.escape(), "ms");
	t.into();
	for (int i = 0; i < N; i++)
	{
		d.pop(vs[i]);
	}

	print("Dict pop time: ", t.escape(), "ms");

	std::unordered_map<std::string, int> u;
	t.into();
	for (int i = 0; i < N; i++)
		u.insert(std::make_pair(vs[i], i));
	print("std::unordered_map insert time: ", t.escape(), "ms");
	t.into();
	for (int i = 0; i < N; i++)
		u[vs[i]];
	print("std::unordered_map query time: ", t.escape(), "ms");
	t.into();
	for (int i = 0; i < N; i++)
		u.erase(vs[i]);
	print("std::unordered_map pop time: ", t.escape(), "ms");
}

void dict_and_or_xor_test()
{
	Dict<int, int> d1{ {1, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 5} };
	Dict<int, int> d2{ {3, 3}, {4, 4}, {5, 5}, {6, 6}, {7, 7} };

	print("d1: ", d1);
	print("d2: ", d2);

	print("d1 & d2: ", d1 & d2);
	print("d1 | d2: ", d1 | d2);
	print("d1 ^ d2: ", d1 ^ d2);

	print("d1 &= d2: ", d1 &= d2);
	print("d1 |= d2: ", d1 |= d2);
	print("d1 ^= d2: ", d1 ^= d2);
}

void dict_test()
{
	Dict<int, int> d{ {1, 1}, {2, 2}, {3, 3}, {4, 4	} };
	print(d.size());
	print("d:", d, "\n");
	print.setend(" ");
	for (auto& k : d.keys())
		print(k);
	print("\n");
	for (auto& v : d.values())
		print(v);
	print("\n");
	for (auto [k, v] : d.items())
		print(k, v);

	print.setend("\n");

	d.insert(5, 5);
	d.insert(6, 6);
	d.insert(7, 7);
	d.insert(8, 8);
	print(d.size());
	print("after insert {5, 5}, {6, 6}, {7, 7}, {8, 8}:", d, "\n");

	print("key 1 value: ", d.get(1));

	d.pop(1);
	d.pop(2);
	print(d.size());
	print("after pop 1, 2:", d, "\n");

	Dict<int, int> d2{ {9, 9} };
	d.update({ {10, 10} });
	d.update(d2);
	print(d.size());
	print("after update {9, 9}, {10, 10}:", d, "\n");

	d.clear();
	print(d.size());
	print("after clear:", d, "\n");
	d.items();
	dict_run_speed_test();
	dict_and_or_xor_test();
}

/*
4
d: {1: 1, 2: 2, 3: 3, 4: 4}

1 2 3 4
 1 2 3 4
 1 1 2 2 3 3 4 4 8
after insert {5, 5}, {6, 6}, {7, 7}, {8, 8}: {1: 1, 2: 2, 3: 3, 4: 4, 5: 5, 6: 6, 7: 7, 8: 8}

key 1 value:  1
6
after pop 1, 2: {3: 3, 4: 4, 5: 5, 6: 6, 7: 7, 8: 8}

8
after update {9, 9}, {10, 10}: {3: 3, 4: 4, 5: 5, 6: 6, 7: 7, 8: 8, 10: 10, 9: 9}

0
after clear: {}

Dict insert time:  2420 ms
Dict query time:  258 ms
Dict pop time:  931 ms
std::unordered_map insert time:  2771 ms
std::unordered_map query time:  273 ms
std::unordered_map pop time:  952 ms
d1:  {1: 1, 2: 2, 3: 3, 4: 4, 5: 5}
d2:  {3: 3, 4: 4, 5: 5, 6: 6, 7: 7}
d1 & d2:  {3: 3, 4: 4, 5: 5}
d1 | d2:  {1: 1, 2: 2, 3: 3, 4: 4, 5: 5, 6: 6, 7: 7}
d1 ^ d2:  {1: 1, 2: 2, 6: 6, 7: 7}
d1 &= d2:  {3: 3, 4: 4, 5: 5}
d1 |= d2:  {3: 3, 4: 4, 5: 5, 6: 6, 7: 7}
d1 ^= d2:  {}
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

## tcp服务器
```cpp
#include <ayr/net.hpp>

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
```

## 简易http服务器
```
#include <ayr/net/http.hpp>

using namespace ayr;

int main()
{
	RequestParser parser;

	Socket http_fd = tcpv4();
	http_fd.bind("127.0.0.1", 7070);
	http_fd.listen();

	auto client_fd = http_fd.accept();

	while (true)
	{
		HttpRequest req;
		Atring req_str;
		do {
			req_str = client_fd.recv(1024);
		} while (!parser(req, req_str));

		print(req.text());

		CString msg = "hello world";
		HttpResponse response("HTTP/1.1", 200, "OK");
		response.add_header("Content-Type", "text/plain");
		response.set_body(msg);

		client_fd.send(response.text());
	}

	client_fd.close();
	http_fd.close();
	return 0;
}
```