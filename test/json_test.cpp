#include <ayr/json.hpp>
#include <ayr/timer.hpp>
#include <ayr/filesystem.hpp>
#include <fstream>

using namespace ayr;

int main()
{
	auto parser = json::JsonParser();
	Timer_ms tm;
	auto twitter_file = fs::join(fs::dirname(__FILE__), "twitter.json");
	Atring a_str = fs::AyrFile(twitter_file, "r").read();
	
	tm.into();
	parser(a_str);
	auto s = tm.escape();
	print("parse twitter.json time elapsed: ", s, "ms\n");
	print.setend("\n\n");
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


	auto j1 = parser(R"({
			  "name": "Alice",
			  "age": 30,
			  "married": true
			})");

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
})");

	tlog(j2);

	auto j3 = parser(R"({
  "tags": ["json", "test", "parser"],
  "scores": [100, 98.5, 76],
  "valid": [true, false, true],
  "misc": [42, "hello", null]
})");
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
})");
	tlog(j4);

	auto j5 = parser(R"({
  "quote": "He said, \"Hello, world!\"",
  "path": "C:\\Users\\test\\file.txt",
  "unicode": "你好，世界"
})");

	tlog(j5);

	json::Json j6 = parser(R"({
  "empty_object": {},
  "empty_array": [],
  "null_value": null,
  "large_number": 9223372036854775807,
  "float_precision": 3.141592653589793
})");

	tlog(j6);
	return 0;
}