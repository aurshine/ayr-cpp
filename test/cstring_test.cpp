#include <array>
#include <string>

#include <ayr/base.hpp>

#include <ayr/base/utest.hpp>

using namespace ayr;

int main()
{
	CString short_view = vstr("hello");
	CString short_copy = short_view.clone();
	CString long_copy = dstr("this string is longer than the sso buffer");
	char* owned_ptr = ayr_alloc<char>(6);
	std::memcpy(owned_ptr, "world", 6);
	CString owned = ostr(owned_ptr, 5);
	UTEST_SCOPE("测试 SSO、视图字符串、深拷贝字符串和外部所有权字符串的基础状态。")
	{
		UTEST_EXPECT(short_view.viewer());
		UTEST_EXPECT(short_copy.sso());
		UTEST_EXPECT(long_copy.owner());
		UTEST_EXPECT(owned.owner());
		UTEST_EXPECT_EQ(owned, "world");
	};

	UTEST_SCOPE("测试拼接、追加、重复和负数下标。")
	{
		CString combined = vstr("hello") + vstr(", ") + owned;
		UTEST_EXPECT_EQ(combined, "hello, world");
		combined += '!';
		UTEST_EXPECT_EQ(combined[-1], '!');
		UTEST_EXPECT_EQ(vstr("ha") * 3, "hahaha");
	};

	CString text = vstr("one two one");
	UTEST_SCOPE("测试查找、反向查找、计数和前后缀。")
	{
		UTEST_EXPECT(text.contains("two"));
		UTEST_EXPECT(text.contains('w'));
		UTEST_EXPECT_EQ(text.index("one"), 0);
		UTEST_EXPECT_EQ(text.index("one", 1), 8);
		UTEST_EXPECT_EQ(text.rindex("one"), 8);
		UTEST_EXPECT_EQ(text.count("one"), 2);
		UTEST_EXPECT_EQ(text.count('o'), 3);
		UTEST_EXPECT(text.startswith("one"));
		UTEST_EXPECT(text.endswith("one"));
		UTEST_EXPECT_EQ(text.index("missing"), -1);
	};

	UTEST_SCOPE("测试 slice 在短字符串和长字符串上的内容正确性。")
	{
		UTEST_EXPECT_EQ(text.slice(0, 3), "one");
		UTEST_EXPECT_EQ(text.slice(4), "two one");
		UTEST_EXPECT_EQ(long_copy.slice(5, 11), "string");
	};

	UTEST_SCOPE("测试大小写转换和字符分类。")
	{
		UTEST_EXPECT_EQ(vstr("AbC").lower(), "abc");
		UTEST_EXPECT_EQ(vstr("AbC").upper(), "ABC");
		UTEST_EXPECT(vstr("123").isdigit());
		UTEST_EXPECT(vstr("abc").isalpha());
		UTEST_EXPECT(vstr(" \t\n").isspace());
	};

	UTEST_SCOPE("测试 join/cjoin 对连续元素和空元素的处理。")
	{
		auto chars = arr(vstr("h"), vstr("e"), vstr("l"), vstr("l"), vstr("o"));
		UTEST_EXPECT_EQ(CString::cjoin(chars), "hello");
		UTEST_EXPECT_EQ(vstr("-").join(chars), "h-e-l-l-o");
		UTEST_EXPECT_EQ(vstr(",").join(arr(vstr("a"), vstr(""), vstr("b"))), "a,,b");
	};

	UTEST_SCOPE("测试 cstr 对常用类型的格式化入口。")
	{
		UTEST_EXPECT_EQ(cstr(nullptr), "nullptr");
		UTEST_EXPECT_EQ(cstr(true), "true");
		UTEST_EXPECT_EQ(cstr(false), "false");
		UTEST_EXPECT_EQ(cstr('x'), "x");
		UTEST_EXPECT_EQ(cstr(42), "42");
	};

	UTEST_SCOPE("测试移动构造后内容仍由新对象持有。")
	{
		CString moved = std::move(long_copy);
		UTEST_EXPECT_EQ(moved, "this string is longer than the sso buffer");
	};

	return UTEST_COMPLETE();
}
