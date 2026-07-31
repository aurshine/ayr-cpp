#include <ayr/filesystem.hpp>
#include <ayr/json.hpp>

#include <ayr/base/utest.hpp>

using namespace ayr;
using namespace ayr::literals;

int main()
{
	UTEST_SCOPE("测试基础 JSON 字面量类型。")
	{
		UTEST_EXPECT(json::loads("null"as).is<json::JsonNull>());
		UTEST_EXPECT_EQ(json::loads("true"as).as<json::JsonBool>(), true);
		UTEST_EXPECT_EQ(json::loads("false"as).as<json::JsonBool>(), false);
		UTEST_EXPECT_EQ(json::loads("123"as).as<json::JsonInt>(), 123);
		UTEST_EXPECT_EQ(json::loads("-42"as).as<json::JsonInt>(), -42);
		UTEST_EXPECT_NEAR(json::loads("3.25"as).as<json::JsonFloat>(), 3.25, 1e-9);
		UTEST_EXPECT_EQ(json::loads(R"("abc")"as).as<json::JsonStr>(), "abc"as);
	};

	UTEST_SCOPE("测试数组解析、负数下标访问、append、pop 和 clear。")
	{
		json::Json arr = json::loads(R"([1, true, "x", null])"as);
		UTEST_EXPECT(arr.is<json::JsonArray>());
		UTEST_EXPECT_EQ(arr.size(), 4);
		UTEST_EXPECT_EQ(arr[0].as<json::JsonInt>(), 1);
		UTEST_EXPECT_EQ(arr[-2].as<json::JsonStr>(), "x"as);
		arr.append(json::Json(json::JsonInt(5)));
		UTEST_EXPECT_EQ(arr[-1].as<json::JsonInt>(), 5);
		arr.pop(1);
		UTEST_EXPECT_EQ(arr.size(), 4);
		arr.clear();
		UTEST_EXPECT_EQ(arr.size(), 0);
	};

	UTEST_SCOPE("测试对象解析、嵌套访问、缺失 key 自动创建和 key 删除。")
	{
		json::Json obj = json::loads(R"({
			"user": {"id": 7, "name": "Alice"},
			"tags": ["json", "unit"],
			"active": true
		})"as);
		UTEST_EXPECT(obj.is<json::JsonDict>());
		UTEST_EXPECT_EQ(obj["user"as]["id"as].as<json::JsonInt>(), 7);
		UTEST_EXPECT_EQ(obj["tags"as][1].as<json::JsonStr>(), "unit"as);
		UTEST_EXPECT_EQ(obj["active"as].as<json::JsonBool>(), true);
		obj["missing"as] = json::Json(json::JsonStr("created"as));
		UTEST_EXPECT_EQ(obj["missing"as].as<json::JsonStr>(), "created"as);
		obj.pop("missing"as);
		UTEST_EXPECT_EQ(obj.size(), 3);
	};

	UTEST_SCOPE("测试字符串中的逗号、括号、转义引号不会打断解析。")
	{
		json::Json tricky = json::loads(R"(["aa", "[[]", "]]", "He said, \"hi\""])"as);
		UTEST_EXPECT_EQ(tricky.size(), 4);
		UTEST_EXPECT_EQ(tricky[1].as<json::JsonStr>(), "[[]"as);
		UTEST_EXPECT_EQ(tricky[3].as<json::JsonStr>(), R"(He said, "hi")"as);
	};

	UTEST_SCOPE("测试JSON字符串转义、Unicode转义和非法转义。")
	{
		json::Json escaped = json::loads(R"("\"\\\/\b\f\n\r\t")"as);
		UTEST_EXPECT_EQ(escaped.as<json::JsonStr>(), "\"\\/\b\f\n\r\t"as);
		UTEST_EXPECT_AYR_ERROR(json::loads(R"("\q")"as));
		UTEST_EXPECT_AYR_ERROR(json::loads("\"\\u" "d83d\""as));
		UTEST_EXPECT_AYR_ERROR(json::loads("\"\\u" "de00\""as));
		UTEST_EXPECT_AYR_ERROR(json::loads("\"line\nbreak\""as));
	};

	UTEST_SCOPE("测试 loads 返回剩余未解析内容。")
	{
		Atring atr = R"({"a": 1} trailing)"as;
		auto [loaded, remain] = json::loads_prefix(atr);
		UTEST_EXPECT_EQ(loaded["a"as].as<json::JsonInt>(), 1);
		UTEST_EXPECT_EQ(remain.strip(), "trailing"as);
	};

	UTEST_SCOPE("测试非法输入和深度限制能抛出 AyrError。")
	{
		UTEST_EXPECT_AYR_ERROR(json::loads("?"as));
		auto old_load_depth = json::JsonLoader::MAX_DEPTH;
		json::JsonLoader::MAX_DEPTH = 3;
		UTEST_EXPR(json::loads(R"({"a": {"b": 1}})"as));
		UTEST_EXPECT_AYR_ERROR(json::loads(R"({"a": {"b": {"c": 1}}})"as));
		json::JsonLoader::MAX_DEPTH = old_load_depth;

		auto old_dump_depth = json::JsonDumper::MAX_DEPTH;
		json::JsonDumper::MAX_DEPTH = 3;
		UTEST_EXPR(json::dumps(json::array({ json::array({ 1 }) })));
		json::Json nested = json::array({ json::array({ json::array({ 1 }) }) });
		UTEST_EXPECT_AYR_ERROR(json::dumps(nested));
		json::JsonDumper::MAX_DEPTH = old_dump_depth;
	};

	UTEST_SCOPE("测试随仓库提供的大 JSON 文件至少能被解析为对象。")
	{
		CString json_dir = fs::join(fs::dirname(__FILE__), "json");
		for (auto&& json_file: fs::listdir(json_dir))
		{
			CString path = fs::join(json_dir, json_file);
			Timer_ms tm;
			tm.into();
			json::Json file_json = json::loads(Atring::from_utf8(fs::AyrFile(path, "r").read()));
			print(json_file, "parse", tm.escape(), "ms");
			UTEST_EXPECT(file_json.is<json::JsonDict>());
			UTEST_EXPECT(file_json.size() > 0);
		}
	};

	UTEST_SCOPE("测试使用自然值构造 JSON 数组和对象。")
	{
		json::Json obj = json::dict({
			{"host"as, "localhost"as},
			{"port"as, 8080},
			{"features"as, json::array({"http"as, "tls"as})}
			});

		UTEST_EXPECT_EQ(obj["host"as].as<json::JsonStr>(), "localhost"as);
		UTEST_EXPECT_EQ(obj["port"as].as<json::JsonInt>(), 8080);
		UTEST_EXPECT_EQ(obj["features"as][0].as<json::JsonStr>(), "http"as);
		UTEST_EXPECT_EQ(obj["features"as][1].as<json::JsonStr>(), "tls"as);

		json::Json alias = json::dict({ {"enabled"as, true} });
		UTEST_EXPECT_EQ(alias["enabled"as].as<json::JsonBool>(), true);
	};

	return UTEST_COMPLETE();
}
