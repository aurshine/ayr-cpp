# c++开发库(开发中)

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
	{
		co_yield n--;
	}
	co_return 100;
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

coro::Task<int> hello()
{
	print("hello");
	co_return 114;
}

coro::Task<int> world()
{
	int h = co_await hello();
	print("hello = ", h);
	print("world");
	co_return 514;
}


int main()
{
	print("world = ", coro::CoroLoop::async_run<coro::Promise<int>>(world()).result());

	return 0;
}

/*
输出:
hello
hello =  114
world
world =  514
*/
```