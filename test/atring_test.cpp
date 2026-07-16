#include <ayr/base.hpp>

#include <ayr/base/utest.hpp>

using namespace ayr;

int main()
{
	// 测试 UTF-8 解码、编码和基础长度。
	Atring hello = Atring::from_utf8("你好世界");
	AYR_TEST_EXPECT_EQ(hello.size(), 4);
	AYR_TEST_EXPECT_EQ(hello.encode(), "你好世界");

	// 测试拼接、追加、重复和负数下标访问。
	Atring digits = "1"as + "2"as + "3"as;
	AYR_TEST_EXPECT_EQ(digits, "123"as);
	AYR_TEST_EXPECT_EQ(digits[0], AChar('2'));
	AYR_TEST_EXPECT_EQ(digits[-1], AChar('3'));
	AYR_TEST_EXPECT_EQ(Atring(AChar('w')) * 3, "www"as);
	digits += "4"as;
	digits += AChar('5');
	AYR_TEST_EXPECT_EQ(digits, "12345"as);

	// 测试查找、反向查找、计数和不存在时的返回值。
	Atring words = "alpha beta alpha"as;
	AYR_TEST_EXPECT(words.contains("beta"as));
	AYR_TEST_EXPECT_EQ(words.index("alpha"as), 0);
	AYR_TEST_EXPECT_EQ(words.index("alpha"as, 1), 11);
	AYR_TEST_EXPECT_EQ(words.rindex("alpha"as), 11);
	AYR_TEST_EXPECT_EQ(words.count("alpha"as), 2);
	AYR_TEST_EXPECT_EQ(words.index("missing"as), -1);

	// 测试视图切片、深拷贝切片和前后缀判断。
	AYR_TEST_EXPECT_EQ(hello.vslice(0, 2), "你好"as);
	AYR_TEST_EXPECT_EQ(hello.slice(2), "世界"as);
	AYR_TEST_EXPECT(hello.startswith("你好"as));
	AYR_TEST_EXPECT(!hello.startswith("世界"as));
	AYR_TEST_EXPECT(hello.endswith("世界"as));
	AYR_TEST_EXPECT(!hello.endswith("你好"as));

	// 测试空白裁剪、指定模式裁剪和替换次数限制。
	AYR_TEST_EXPECT_EQ("  hello \t"as.strip(), "hello"as);
	AYR_TEST_EXPECT_EQ("xxhelloxx"as.strip("x"as), "hello"as);
	AYR_TEST_EXPECT_EQ("xxhelloxx"as.lstrip("x"as), "helloxx"as);
	AYR_TEST_EXPECT_EQ("xxhelloxx"as.rstrip("x"as), "xxhello"as);
	AYR_TEST_EXPECT_EQ("one two two"as.replace("two"as, "2"as, 1), "one 2 two"as);

	// 测试 join、按分隔符 split、按空白 split 和 maxsplit。
	AYR_TEST_EXPECT_EQ(","as.join(arr("我"as, "爱"as, "你"as)), "我,爱,你"as);
	auto csv = "a,b,c"as.split(","as);
	AYR_TEST_EXPECT_EQ(csv.size(), 3);
	AYR_TEST_EXPECT_EQ(csv[0], "a"as);
	AYR_TEST_EXPECT_EQ(csv[2], "c"as);
	auto limited = "a,b,c"as.split(","as, 1);
	AYR_TEST_EXPECT_EQ(limited.size(), 2);
	AYR_TEST_EXPECT_EQ(limited[1], "b,c"as);
	auto by_space = "a  b\tc"as.split();
	AYR_TEST_EXPECT_EQ(by_space.size(), 3);
	AYR_TEST_EXPECT_EQ(by_space[1], "b"as);

	// 测试大小写、字符分类、整数/浮点解析及剩余字符串返回。
	AYR_TEST_EXPECT_EQ("AbC"as.lower(), "abc"as);
	AYR_TEST_EXPECT_EQ("AbC"as.upper(), "ABC"as);
	AYR_TEST_EXPECT("12345"as.isdigit());
	AYR_TEST_EXPECT("abc"as.isalpha());
	auto [int_value, int_remain] = "-101z"as.toint(2);
	AYR_TEST_EXPECT_EQ(int_value, -5);
	AYR_TEST_EXPECT_EQ(int_remain, "z"as);
	auto [float_value, float_remain] = "-12.5kg"as.tofloat();
	AYR_TEST_EXPECT_NEAR(float_value, -12.5, 1e-9);
	AYR_TEST_EXPECT_EQ(float_remain, "kg"as);
}
