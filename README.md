# c++开发库(开发中)

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