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