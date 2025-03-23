#include <ayr/json.hpp>

using namespace ayr;

int main()
{
	tlog(json::parse("123"));
	tlog(json::parse("true"));
	tlog(json::parse("false"));
	tlog(json::parse("null"));
	tlog(json::parse(R"("abc")"));
	tlog(json::parse("[]"));
	tlog(json::parse(R"(["1", "2", "3"])"));
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
	for (auto& item : json_obj["array"as].as_array())
		tlog(item);

	return 0;
}