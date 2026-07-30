#include <ayr/filesystem.hpp>
#include <ayr/json.hpp>

#include <ayr/base/utest.hpp>

using namespace ayr;
using namespace ayr::literals;

int main()
{
	UTEST_SCOPE("测试基础 JSON 字面量类型。")
	{
		UTEST_EXPECT(json::load("null"as).is_null());
		UTEST_EXPECT_EQ(json::load("true"as).as_bool(), true);
		UTEST_EXPECT_EQ(json::load("false"as).as_bool(), false);
		UTEST_EXPECT_EQ(json::load("123"as).as_int(), 123);
		UTEST_EXPECT_EQ(json::load("-42"as).as_int(), -42);
		UTEST_EXPECT_NEAR(json::load("3.25"as).as_float(), 3.25, 1e-9);
		UTEST_EXPECT_EQ(json::load(R"("abc")"as).as_str(), "abc"as);
	};

	UTEST_SCOPE("测试数组解析、负数下标访问、append、pop 和 clear。")
	{
		json::Json arr = json::load(R"([1, true, "x", null])"as);
		UTEST_EXPECT(arr.is_array());
		UTEST_EXPECT_EQ(arr.size(), 4);
		UTEST_EXPECT_EQ(arr[0].as_int(), 1);
		UTEST_EXPECT_EQ(arr[-2].as_str(), "x"as);
		arr.append(json::Json(json::JsonInt(5)));
		UTEST_EXPECT_EQ(arr[-1].as_int(), 5);
		arr.pop(1);
		UTEST_EXPECT_EQ(arr.size(), 4);
		arr.clear();
		UTEST_EXPECT_EQ(arr.size(), 0);
	};

	UTEST_SCOPE("测试对象解析、嵌套访问、缺失 key 自动创建和 key 删除。")
	{
		json::Json obj = json::load(R"({
			"user": {"id": 7, "name": "Alice"},
			"tags": ["json", "unit"],
			"active": true
		})"as);
		UTEST_EXPECT(obj.is_dict());
		UTEST_EXPECT_EQ(obj["user"as]["id"as].as_int(), 7);
		UTEST_EXPECT_EQ(obj["tags"as][1].as_str(), "unit"as);
		UTEST_EXPECT_EQ(obj["active"as].as_bool(), true);
		obj["missing"as] = json::Json(json::JsonStr("created"as));
		UTEST_EXPECT_EQ(obj["missing"as].as_str(), "created"as);
		obj.pop("missing"as);
		UTEST_EXPECT_EQ(obj.size(), 3);
	};

	UTEST_SCOPE("测试字符串中的逗号、括号、转义引号不会打断解析。")
	{
		json::Json tricky = json::load(R"(["aa", "[[]", "]]", "He said, \"hi\""])"as);
		UTEST_EXPECT_EQ(tricky.size(), 4);
		UTEST_EXPECT_EQ(tricky[1].as_str(), "[[]"as);
		UTEST_EXPECT(tricky[3].as_str().contains(R"(\"hi\")"as));
	};

	UTEST_SCOPE("测试 loads 返回剩余未解析内容。")
	{
		Atring atr = R"({"a": 1} trailing)"as;
		auto [loaded, remain] = json::loads(atr);
		UTEST_EXPECT_EQ(loaded["a"as].as_int(), 1);
		UTEST_EXPECT_EQ(remain.strip(), "trailing"as);
	};

	UTEST_SCOPE("测试非法输入和深度限制能抛出 AyrError。")
	{
		UTEST_EXPECT_AYR_ERROR(json::load("?"as));
		auto old_depth = json::JsonLoader::MAX_DEPTH;
		json::JsonLoader::MAX_DEPTH = 2;
		UTEST_EXPECT_AYR_ERROR(json::load(R"({"a": {"b": {"c": 1}}})"as));
		json::JsonLoader::MAX_DEPTH = old_depth;
	};

	UTEST_SCOPE("测试随仓库提供的大 JSON 文件至少能被解析为对象。")
	{
		CString json_dir = fs::join(fs::dirname(__FILE__), "json");
		for (auto&& json_file: fs::listdir(json_dir))
		{
			CString path = fs::join(json_dir, json_file);
			Timer_ms tm;
			tm.into();
			json::Json file_json = json::load(Atring::from_utf8(fs::AyrFile(path, "r").read()));
			print(json_file, "parse", tm.escape(), "ms");
			UTEST_EXPECT(file_json.is_dict());
			UTEST_EXPECT(file_json.size() > 0);
		}
	};

	return UTEST_COMPLETE();
}
