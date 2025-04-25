#include <ayr/json.hpp>

using namespace ayr;

int main()
{
	auto parser = json::JsonParser();

	tlog(parser("123"));
	tlog(parser("true"));
	tlog(parser("false"));
	tlog(parser("null"));
	tlog(parser(R"("abc")"));
	tlog(parser("[]"));
	tlog(parser(R"(["1", "2", "3"])"));
	tlog(parser("{}"));
	tlog(parser(R"({"a": 1})"));
	tlog(parser(R"({"a": 1, "b": 2})"));
	tlog(parser(R"({"a": 1, "b": [2, 3]})"));
	tlog(parser(R"({"a": 1, "b": {"c": 2}})"));
	tlog(parser(R"({"a": 1, "b": {"c": 2, "d": [3, 4]}})"));
	tlog(parser(R"("abc\\"")"));
	tlog(parser(R"(["aa", "[[]", "]]", "{}"])"));

	Atring json_str = R"({"array": [1, 2, "3", 4, 5.6, ["a", "b", "c"], {"d": 1, "e": 2, "f": 3}]})";
	tlog(json_str);
	json::Json json_obj = parser(json_str);
	tlog(json_obj);
	for (auto& item : json_obj["array"as].as_array())
		tlog(item);

	return 0;
}