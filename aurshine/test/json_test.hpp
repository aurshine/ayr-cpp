#include <ayr/json.hpp>

using namespace ayr;

void json_test()
{
	Atring json_str = R"({"array": [1, 2, "3", 4, 5.6, ["a", "b", "c"], {"d": 1, "e": 2, "f": 3}]})";
	json::Json json_obj = json::parse(json_str);

	for (auto& item : json_obj["array"as].transform<json::JsonType::JsonArray>())
		print(item);
}