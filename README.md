# c++开发库(开发中)

## json 格式解析
在json/parse.hpp中实现对json格式的字符串解析为一个json对象
```cpp
#include <string>
#include "law/printer.hpp"
#include "json/parse.hpp"

int main()
{
	std::string json_str = R"({"array": [1, 2, "3", 4.0, true, null],})";
	ayr::Json json_obj = ayr::parse(json_str);

	ayr::print(json_obj);
	return 0;
}
```