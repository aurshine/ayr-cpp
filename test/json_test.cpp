#include <ayr/json.hpp>
#include <ayr/filesystem.hpp>

using namespace ayr;

using namespace ayr::literals;

int main()
{
	Timer_ms tm;
	auto twitter_file = fs::join(fs::dirname(__FILE__), "json/canada.json");
	auto datas = fs::AyrFile(twitter_file, "r").read();

	auto a_str = Atring::from_utf8(datas);
	tm.into();
	json::load(a_str);
	auto s = tm.escape();
	print("parse canada.json time elapsed: ", s, "ms\n");
	return 0;
	print.setend("\n\n");
	tlog(json::load("123"as));
	tlog(json::load("true"as));
	tlog(json::load("false"as));
	tlog(json::load("null"as));
	tlog(json::load(R"("abc")"as));
	tlog(json::load("[]"as));
	tlog(json::load(R"(["1", "2", "3"])"as));
	tlog(json::load("{}"as));
	tlog(json::load(R"({"a": 1})"as));
	tlog(json::load(R"({"a": 1, "b": 2})"as));
	tlog(json::load(R"({"a": 1, "b": [2, 3]})"as));
	tlog(json::load(R"({"a": 1, "b": {"c": 2}})"as));
	tlog(json::load(R"({"a": 1, "b": {"c": 2, "d": [3, 4]}})"as));
	tlog(json::load(R"("abc\\"")"as));
	tlog(json::load(R"(["aa", "[[]", "]]", "{}"])"as));

	Atring json_str = R"({"array": [1, 2, "3", 4, 5.6, ["a", "b", "c"], {"d": 1, "e": 2, "f": 3}]})"as;
	tlog(json_str);
	json::Json json_obj = json::load(json_str);
	tlog(json_obj);
	for (auto& item : json_obj["array"as].as_array())
		tlog(item);


	auto j1 = json::load(R"({
			  "name": "Alice",
			  "age": 30,
			  "married": true
			})"as);

	tlog(j1);

	auto j2 = json::load(R"({
  "user": {
	"id": 101,
	"info": {
	  "first_name": "Bob",
	  "last_name": "Smith"
	}
  },
  "active": false
})"as);

	tlog(j2);

	auto j3 = json::load(R"({
  "tags": ["json", "test", "json::load"],
  "scores": [100, 98.5, 76],
  "valid": [true, false, true],
  "misc": [42, "hello", null]
})"as);
	tlog(j3);

	auto j4 = json::load(R"({
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
})"as);
	tlog(j4);

	auto j5 = json::load(R"({
  "quote": "He said, \"Hello, world!\"",
  "path": "C:\\Users\\test\\file.txt",
  "unicode": "你好，世界"
})"as);

	tlog(j5);

	json::Json j6 = json::load(R"({
  "empty_object": {},
  "empty_array": [],
  "null_value": null,
  "large_number": 9223372036854775807,
  "float_precision": 3.141592653589793
})"as);

	tlog(j6);
	return 0;
}