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
pass time: 1395.45 ms
test_vs
pass time: 2782.59 ms
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

## socket回声服务器
```
void client()
{
	constexpr int BUFFER_SIZE = 1024;
	char input_data[BUFFER_SIZE];

	Socket client(AF_INET, SOCK_STREAM);
	client.connect(nullptr, 14514);
	while (true)
	{
		print.setend(" ");
		print("client input message:");
		print.setend("\n");
		std::cin >> input_data;
		if (input_data[0] == 'q' || input_data[0] == 'Q')
			break;
		client.send(input_data, strlen(input_data));
		print("server recv message:", client.recv());
		print();
	}
}

void server()
{
	Socket server(AF_INET, SOCK_STREAM);

	server.bind(nullptr, 14514);

	Socket client = server.accept();
	while (true)
	{
		CString msg = client.recv();
		if (!msg) break;
		client.send(msg.data(), msg.size());
	}
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