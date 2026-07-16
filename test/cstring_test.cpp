#include <array>
#include <string>

#include <ayr/base.hpp>

#include <ayr/base/utest.hpp>

using namespace ayr;

int main()
{
	// 测试 SSO、视图字符串、深拷贝字符串和外部所有权字符串的基础状态。
	CString short_view = vstr("hello");
	CString short_copy = short_view.clone();
	CString long_copy = dstr("this string is longer than the sso buffer");
	char* owned_ptr = ayr_alloc<char>(6);
	std::memcpy(owned_ptr, "world", 6);
	CString owned = ostr(owned_ptr, 5);
	AYR_TEST_EXPECT(short_view.viewer());
	AYR_TEST_EXPECT(short_copy.sso());
	AYR_TEST_EXPECT(long_copy.owner());
	AYR_TEST_EXPECT(owned.owner());
	AYR_TEST_EXPECT_EQ(owned, "world");

	// 测试拼接、追加、重复和负数下标。
	CString combined = vstr("hello") + vstr(", ") + owned;
	AYR_TEST_EXPECT_EQ(combined, "hello, world");
	combined += '!';
	AYR_TEST_EXPECT_EQ(combined[-1], '!');
	AYR_TEST_EXPECT_EQ(vstr("ha") * 3, "hahaha");

	// 测试查找、反向查找、计数和前后缀。
	CString text = vstr("one two one");
	AYR_TEST_EXPECT(text.contains("two"));
	AYR_TEST_EXPECT(text.contains('w'));
	AYR_TEST_EXPECT_EQ(text.index("one"), 0);
	AYR_TEST_EXPECT_EQ(text.index("one", 1), 8);
	AYR_TEST_EXPECT_EQ(text.rindex("one"), 8);
	AYR_TEST_EXPECT_EQ(text.count("one"), 2);
	AYR_TEST_EXPECT_EQ(text.count('o'), 3);
	AYR_TEST_EXPECT(text.startswith("one"));
	AYR_TEST_EXPECT(text.endswith("one"));
	AYR_TEST_EXPECT_EQ(text.index("missing"), -1);

	// 测试 slice/vslice 在短字符串和长字符串上的内容正确性。
	AYR_TEST_EXPECT_EQ(text.vslice(0, 3), "one");
	AYR_TEST_EXPECT_EQ(text.slice(4), "two one");
	AYR_TEST_EXPECT_EQ(long_copy.vslice(5, 11), "string");

	// 测试大小写转换和字符分类。
	AYR_TEST_EXPECT_EQ(vstr("AbC").lower(), "abc");
	AYR_TEST_EXPECT_EQ(vstr("AbC").upper(), "ABC");
	AYR_TEST_EXPECT(vstr("123").isdigit());
	AYR_TEST_EXPECT(vstr("abc").isalpha());
	AYR_TEST_EXPECT(vstr(" \t\n").isspace());

	// 测试 join/cjoin 对连续元素和空元素的处理。
	auto chars = arr(vstr("h"), vstr("e"), vstr("l"), vstr("l"), vstr("o"));
	AYR_TEST_EXPECT_EQ(CString::cjoin(chars), "hello");
	AYR_TEST_EXPECT_EQ(vstr("-").join(chars), "h-e-l-l-o");
	AYR_TEST_EXPECT_EQ(vstr(",").join(arr(vstr("a"), vstr(""), vstr("b"))), "a,,b");

	// 测试 cstr 对常用类型的格式化入口。
	AYR_TEST_EXPECT_EQ(cstr(nullptr), "nullptr");
	AYR_TEST_EXPECT_EQ(cstr(true), "true");
	AYR_TEST_EXPECT_EQ(cstr(false), "false");
	AYR_TEST_EXPECT_EQ(cstr('x'), "x");
	AYR_TEST_EXPECT_EQ(cstr(42), "42");

	// 测试移动构造后内容仍由新对象持有。
	CString moved = std::move(long_copy);
	AYR_TEST_EXPECT_EQ(moved, "this string is longer than the sso buffer");
}
