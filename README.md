﻿# c++开发库(开发中)

## json 格式解析
在json/parse.hpp中实现对json格式的字符串解析为一个json对象
```cpp
#include <law/AString.hpp>
#include "law/printer.hpp"
#include "json/parse.hpp"

int main()
{
	ayr::Astring json_str = R"({"array": [1, 2, "3", 4.0, true, null],})";
	ayr::json::Json json_obj = ayr::json::parse(json_str);

	for (auto& item : json_obj[ayr::Astring("array")].transform<typename ayr::json::JsonType::JsonArray>())
	{
		ayr::print(item);
	}
	return 0;
}
```

## 动态数组 DynArray
对于拷贝昂贵的类型，DynArray远快于vector
```cpp
#include <vector>
#include <string>
#include <law/Array.hpp>
#include <law/DynArray.hpp>
#include <law/timer.hpp>

ayr::DynArray<std::string> das;
std::vector<std::string> vs;

constexpr int N = 100000;

void test_das()
{
	for (int i = 0; i < N; ++i)
	{
		das.append("hello");
	}
}

void test_vs()
{
	for (int i = 0; i < N; ++i)
	{
		vs.push_back("hello");
	}
}

int main()
{
	auto tm = ayr::Timer();

	tm(test_das);
	tm(test_vs);

	return 0;
}

/*
输出：
Time elapsed: 63251 microseconds
Time elapsed: 169696 microseconds
*/
```