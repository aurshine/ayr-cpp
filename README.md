# c++开发库(开发中)

## json 格式解析
在json/parse.hpp中实现对json格式的字符串解析为一个json对象
```cpp
#include <json/parse.hpp>

using namespace ayr;


int main()
{
	Atring json_str = R"({"array": [1, 2, "3", 4, 5.6, ["a", "b", "c"], {"d": 1, "e": 2, "f": 3}]})";
	json::Json json_obj = ayr::json::parse(json_str);

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
#include <law/Array.hpp>
#include <law/DynArray.hpp>
#include <law/timer.hpp>

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
输出：
test_das
pass time: 1395.45 ms
test_vs
pass time: 2782.59 ms
*/
```