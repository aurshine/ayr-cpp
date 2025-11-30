1. # **ayr — augment your runtime**

   **ayr** 是一个以“简洁语法 + 轻量高效”为理念的现代 C++20 工具库。它提供标准库未支持的能力，同时为许多已有能力提供更快、更易用、更一致的替代方案。

   设计目标简单直接：

   - 写 C++ 的时候别被语法拖住手脚
   - 在不牺牲表达力的前提下尽可能高性能
   - 将常用工具封装为现代、统一、可编译期使用的组件

## CString字节串

c风格的字节串，可以从`char* std::string std::string_view`快速切换，并做到0开销

```cpp
const char* a = "123";
std::string b = "123";
std::string_view c = "123";

// 构建视图，不拷贝内容
vstr(a);
vstr(b);
vstr(c);

// 拷贝数据，重新构造对象
dstr(a);
dstr(b);
dstr(c);
```



## Atring字符串
unicode编码的字符串

提供比std::string更加高效，操作更加丰富的字符串

90%方法支持编译期调用
```cpp
constexpr Atring str0 = "atring"as
constexpr Atring str1 = Atring::from_utf8("你好世界");
constexpr Atring str2 = "1"as + "2"as + "3"as;
constexpr Atring str3 = AChar('w');
constexpr Atring str4 = str3 * 3;
Atring str5 = "你好世界我很好"as;
str5 += "你好世界"as;
constexpr AChar str2_0 = str2[0];
constexpr AChar str2__1 = str2[-1];
constexpr bool contains = str1.contains("你好"as);
constexpr c_size idx = str4.index("w"as);
constexpr c_size ridx = str4.rindex("w"as);
constexpr c_size cnt = str4.count("w"as);
constexpr Atring str6 = str1.vslice(0, 2);
constexpr Atring str7 = str1.vslice(2);
constexpr bool sw1 = str1.startswith("你好"as);
constexpr bool sw2 = str1.startswith("世界"as);
constexpr bool ew1 = str1.endswith("世界"as);
constexpr bool ew2 = str1.endswith("你好"as);
constexpr Atring str8 = str1.strip("你好"as);
constexpr Atring str9 = str1.lstrip("你好"as);
constexpr Atring str10 = str1.rstrip("世界"as);
constexpr Atring str11 = str1.replace("你好"as, "你们好"as);
constexpr Atring str12 = ","as.join(std::array<Atring, 3>{ "我"as, "爱"as, "你"as });
str12.split(","as);
" 你 好 世 界 "as.split();
constexpr c_size num1 = "114514"as.to_int();
constexpr c_size num2 = "-11451"as.to_int();
constexpr double num3 = "114.51"as.to_double();
constexpr double num4 = "-114.5"as.to_double();
}
```

## json 解析
根据json字符串快速解析出json对象，支持多种json格式
```cpp
#include <ayr/json.hpp>
#include <ayr/filesystem.hpp>

using namespace ayr;

using namespace ayr::literals;

int main()
{
	auto parser = json::JsonParser();
	Timer_ms tm;
	auto twitter_file = fs::join(fs::dirname(__FILE__), "twitter.json");
	auto datas = fs::AyrFile(twitter_file, "r").read();

	Atring a_str = Atring::from_utf8(datas);
	tm.into();
	parser(a_str);
	auto s = tm.escape();
	print("parse twitter.json time elapsed: ", s, "ms\n");
	print.setend("\n\n");
	tlog(parser("123"as));
	tlog(parser("true"as));
	tlog(parser("false"as));
	tlog(parser("null"as));
	tlog(parser(R"("abc")"as));
	tlog(parser("[]"as));
	tlog(parser(R"(["1", "2", "3"])"as));
	tlog(parser("{}"as));
	tlog(parser(R"({"a": 1})"as));
	tlog(parser(R"({"a": 1, "b": 2})"as));
	tlog(parser(R"({"a": 1, "b": [2, 3]})"as));
	tlog(parser(R"({"a": 1, "b": {"c": 2}})"as));
	tlog(parser(R"({"a": 1, "b": {"c": 2, "d": [3, 4]}})"as));
	tlog(parser(R"("abc\\"")"as));
	tlog(parser(R"(["aa", "[[]", "]]", "{}"])"as));

	Atring json_str = R"({"array": [1, 2, "3", 4, 5.6, ["a", "b", "c"], {"d": 1, "e": 2, "f": 3}]})"as;
	tlog(json_str);
	json::Json json_obj = parser(json_str);
	tlog(json_obj);
	for (auto& item : json_obj["array"as].as_array())
		tlog(item);


	auto j1 = parser(R"({
			  "name": "Alice",
			  "age": 30,
			  "married": true
			})"as);

	tlog(j1);

	auto j2 = parser(R"({
  "user": {
	"id": 101,
	"info": {
	  "first_name": "Bob",
	  "last_name": "Smith"
	}
  },
  "active": false
})"as);

	tlog(j2);

	auto j3 = parser(R"({
  "tags": ["json", "test", "parser"],
  "scores": [100, 98.5, 76],
  "valid": [true, false, true],
  "misc": [42, "hello", null]
})"as);
	tlog(j3);

	auto j4 = parser(R"({
  "company": {
	"name": "OpenAI",
	"departments": [
	  {
		"name": "Research",
		"employees": [
		  {"name": "Alice", "id": 1},
		  {"name": "Bob", "id": 2}
		]
	  },
	  {
		"name": "Engineering",
		"employees": []
	  }
	]
  }
})"as);
	tlog(j4);

	auto j5 = parser(R"({
  "quote": "He said, \"Hello, world!\"",
  "path": "C:\\Users\\test\\file.txt",
  "unicode": "你好，世界"
})"as);

	tlog(j5);

	json::Json j6 = parser(R"({
  "empty_object": {},
  "empty_array": [],
  "null_value": null,
  "large_number": 9223372036854775807,
  "float_precision": 3.141592653589793
})"as);

	tlog(j6);
	return 0;
}
}
```

## 动态数组 DynArray
对于拷贝昂贵的类型，DynArray远快于vector
```cpp
#include <vector>
#include <string>
#include <algorithm>

#include <ayr/air/DynArray.hpp>

using namespace ayr;

ayr::DynArray<std::string> das;
std::vector<std::string> vs;
ayr::DynArray<int> dai;
std::vector<int> vi;

constexpr int N = 1e6;

void test_das_add()
{
	for (int i = 0; i < N; ++i)
		das.append("hello");
}

void test_vs_add()
{
	for (int i = 0; i < N; ++i)
		vs.push_back("hello");
}

void test_dai_add()
{
	for (int i = 0; i < N; ++i)
		dai.append(i);
}

void test_vi_add()
{
	for (int i = 0; i < N; ++i)
		vi.push_back(i);
}

void test_dai_query()
{
	for (int i = 0; i < N; ++i)
		assert(dai[i] == i);
}

void test_vi_query()
{
	for (int i = 0; i < N; ++i)
		assert(vi[i] == i);
}


void runspeed_test()
{
	Timer_ms tm;

	print("dynarray append str time:", tm(test_das_add), "ms");
	print("vector push_back str time:", tm(test_vs_add), "ms");
	print("dayarray append int time:", tm(test_dai_add), "ms");
	print("vector push_back int time:", tm(test_vi_add), "ms");
	print("dayarray query int time:", tm(test_dai_query), "ms");
	print("vector query str time:", tm(test_vi_query), "ms");
}

/*
输出:
dynarray append str time: 9.260300 ms
vector push_back str time: 50.623700 ms
dayarray append int time: 4.429500 ms
vector push_back int time: 5.416800 ms
dayarray query int time: 0.000200 ms
vector query str time: 0.000100 ms
*/
```
## Dict
提供比unordered_map更丰富更高效的操作
```cpp
#include <unordered_map>

#include <ayr/air/Dict.hpp>

using namespace ayr;

void dict_run_speed_test()
{
	Timer_ms t;
	Dict<std::string, std::string> d;
	std::vector<std::string> vs;
	constexpr int N = 1e6;

	for (int i = 0; i < N; i++)
		vs.push_back(std::to_string(i));

	t.into();
	for (int i = 0; i < N; i++)
		d[vs[i]] = vs[i];
	print("Dict insert time: ", t.escape(), "ms");
	t.into();
	for (int i = 0; i < N; i++)
		assert(d.contains(vs[i]));
	print("Dict query time: ", t.escape(), "ms");
	t.into();
	for (int i = 0; i < N; i++)
	{
		d.pop(vs[i]);
	}

	print("Dict pop time: ", t.escape(), "ms");

	std::unordered_map<std::string, std::string> u;
	t.into();
	for (int i = 0; i < N; i++)
		u.insert(std::make_pair(vs[i], vs[i]));
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

int main()
{
	dict_run_speed_test();
	return 0;
}
/*
输出:
Dict insert time:  243.844200 ms
Dict query time:  0.000000 ms
Dict pop time:  217.485900 ms
std::unordered_map insert time:  417.110600 ms
std::unordered_map query time:  137.973500 ms
std::unordered_map pop time:  180.377100 m
```

## Set
提供比unordered_set更丰富更高效的操作
```cpp

#include <unordered_set>

#include <ayr/air/Set.hpp>

using namespace ayr;

void set_run_speed_test()
{
	Timer_ms t;
	std::vector<std::string> vs;
	constexpr int N = 1e6;
	for (int i = 0; i < N; i++)
		vs.push_back(std::to_string(i));

	Set<c_size> s;
	t.into();
	for (int i = 0; i < N; i++)
		s.insert(i);
	print("Set insert time: ", t.escape(), "ms");
	t.into();
	for (int i = 0; i < N; i++)
		assert(s.contains(i));

	print("Set query time: ", t.escape(), "ms");
	t.into();
	for (int i = 0; i < N; i++)
		s.pop(i);
	print("Set pop time: ", t.escape(), "ms");

	std::unordered_set<c_size> u;
	t.into();
	for (int i = 0; i < N; i++)
		u.insert(i);
	print("std::unordered_set insert time: ", t.escape(), "ms");
	t.into();
	for (int i = 0; i < N; i++)
		assert(u.count(i));
	print("std::unordered_set query time: ", t.escape(), "ms");
	t.into();
	for (int i = 0; i < N; i++)
		u.erase(i);
	print("std::unordered_set pop time: ", t.escape(), "ms");
}

int main()
{
	set_run_speed_test();
	return 0;
}
/*
输出:
Set insert time:  191.576000 ms
Set query time:  0.000000 ms
Set pop time:  149.241800 ms
std::unordered_set insert time:  319.408700 ms
std::unordered_set query time:  0.013200 ms
std::unordered_set pop time:  194.372400 ms
*/
```

## 文件操作
```cpp
#include "ayr/filesystem.hpp"

using namespace ayr;

int main()
{
	fs::AyrFile af(__FILE__, "r");

	for (auto line : af.readlines())
	{
		print(line);
		//std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	return 0;
	CString file_name = fs::join(fs::dirname(__FILE__), "test.txt");
	tlog(fs::exists(file_name));
	fs::AyrFile a(file_name, "w");
	a.close();
	tlog(fs::exists(file_name));
	tlog(fs::isfile(file_name));
	tlog(fs::isdir(file_name));
	fs::remove(file_name);
	tlog(fs::exists(file_name));

	CString dir_name = fs::join(fs::dirname(__FILE__), "AAAAA_test_dir");
	tlog(fs::exists(dir_name));
	fs::mkdir(dir_name);
	tlog(fs::exists(dir_name));
	tlog(fs::isfile(dir_name));
	tlog(fs::isdir(dir_name));
	fs::remove(dir_name);
	tlog(fs::exists(dir_name));

	tlog(fs::join("home", "file.txt"));
	tlog(fs::join("/home", "/file.txt"));
	tlog(fs::basename("/home/user/file.txt"));
	tlog(fs::basename("a.txt"));
	tlog(fs::basename("/"));
	tlog(fs::dirname("/home/user/file.txt"));
	tlog(fs::dirname("a.txt"));
	tlog(fs::dirname("/"));
	tlog(fs::splitext("/home/user/file.txt"));
	tlog(fs::splitext("a.txt"));
	tlog(fs::splitext("abc"));
	tlog(fs::split("/home/user/file.txt"));
	tlog(fs::split("a.txt"));
	tlog(fs::split("abc"));
	tlog(fs::getcwd());

	print("\n[");
	for (auto& p : fs::listdir(fs::getcwd()))
		print(p);
	print("]");

	print("\n{");
	for (auto& [root, dirs, files] : fs::walk(fs::getcwd()))
	{
		tlog(root);
		tlog(dirs);
		tlog(files);
		print("\n");
	}
	print("}");
}
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

## tcp echo服务器
```cpp
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

/*
输出:

*/server start
server bind 127.0.0.1 : 7777
server listening
client start
client connected to server:  Socket(260)
server accepted client:  Socket(184)
server received:  Hello, world! 13 bytes
client sent: Hello, world!
client received:  Hello, world!
server end
client end
```

## http请求
```
#include <ayr/net/http.hpp>

using namespace ayr;

coro::Task<void> task(coro::IoContext* io_context)
{
	auto u = net::uri("http://www.baidu.com"as);

	net::HttpResponse res = co_await net::get(io_context, u);

	print(res.text());
}

int main()
{
	coro::IoContext io_context;
	io_context.run(task(&io_context));
	return 0;
}
```