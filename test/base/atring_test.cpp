#include <ayr/base.hpp>

#include <cmath>
#include <limits>
#include <type_traits>

#include <ayr/base/utest.hpp>

using namespace ayr;

constexpr bool atring_sso_constexpr_lifecycle()
{
	Atring source(AChar('a'));
	Atring copy(source);
	copy += AChar('b');

	Atring assigned;
	assigned = copy;

	Atring moved(std::move(assigned));
	Atring move_assigned;
	move_assigned = std::move(moved);

	return source == Atring(AChar('a'))
		&& copy.size() == 2
		&& move_assigned.size() == 2
		&& move_assigned[0] == AChar('a')
		&& move_assigned[1] == AChar('b')
		&& assigned.empty()
		&& moved.empty();
}

static_assert(atring_sso_constexpr_lifecycle());
static_assert(std::is_same_v<decltype(Atring{}.toint()), c_size>);
static_assert(std::is_same_v<decltype(Atring{}.tofloat()), double>);

int main()
{
	Atring hello = Atring::from_utf8("你好世界");
	UTEST_SCOPE("测试 UTF-8 解码、编码和基础长度。")
	{
		UTEST_EXPECT_EQ(hello.size(), 4);
		UTEST_EXPECT_EQ(hello.encode(), "你好世界");
	};

	UTEST_SCOPE("测试拼接、追加、重复和负数下标访问。")
	{
		Atring digits = "1"as + "2"as + "3"as;
		UTEST_EXPECT_EQ(digits, "123"as);
		UTEST_EXPECT_EQ(digits[0], AChar('1'));
		UTEST_EXPECT_EQ(digits[-1], AChar('3'));
		UTEST_EXPECT_EQ(Atring(AChar('w')) * 3, "www"as);
		digits += "4"as;
		digits += AChar('5');
		UTEST_EXPECT_EQ(digits, "12345"as);
	};

    UTEST_SCOPE("测试空字符串与短字符串、长字符串和共享切片拼接。")
    {
        Atring empty;
        UTEST_EXPECT((empty + empty).empty());
        UTEST_EXPECT_EQ(empty + "abc"as, "abc"as);
        UTEST_EXPECT_EQ("abc"as + empty, "abc"as);

        Atring left, right, sliced;
        {
            Atring source = "0123456789abcdef"as;
            left = empty + source;
            right = source + empty;
            sliced = empty + source.slice(2, 14);
            source = "changed"as;
        }
        UTEST_EXPECT_EQ(left, "0123456789abcdef"as);
        UTEST_EXPECT_EQ(right, "0123456789abcdef"as);
        UTEST_EXPECT_EQ(sliced, "23456789abcd"as);
        left += AChar('!');
        UTEST_EXPECT_EQ(left, "0123456789abcdef!"as);
        UTEST_EXPECT_EQ(right, "0123456789abcdef"as);
        UTEST_EXPECT(empty.empty());
    };

	UTEST_SCOPE("测试查找、反向查找、计数和不存在时的返回值。")
	{
		Atring words = "alpha beta alpha"as;
		UTEST_EXPECT(words.contains("beta"as));
		UTEST_EXPECT_EQ(words.index("alpha"as), 0);
		UTEST_EXPECT_EQ(words.index("alpha"as, 1), 11);
		UTEST_EXPECT_EQ(words.rindex("alpha"as), 11);
		UTEST_EXPECT_EQ(words.count("alpha"as), 2);
		UTEST_EXPECT_EQ(words.index("missing"as), -1);
	};

	UTEST_SCOPE("测试视图切片、深拷贝切片和前后缀判断。")
	{
		UTEST_EXPECT_EQ(hello.slice(0, 2), "你好"as);
		UTEST_EXPECT_EQ(hello.slice(2), "世界"as);
		UTEST_EXPECT(hello.startswith("你好"as));
		UTEST_EXPECT(!hello.startswith("世界"as));
		UTEST_EXPECT(hello.endswith("世界"as));
		UTEST_EXPECT(!hello.endswith("你好"as));
	};

	UTEST_SCOPE("测试空白裁剪、指定模式裁剪和替换次数限制。")
	{
		UTEST_EXPECT_EQ("  hello \t"as.strip(), "hello"as);
		UTEST_EXPECT_EQ("xxhelloxx"as.strip("x"as), "hello"as);
		UTEST_EXPECT_EQ("xxhelloxx"as.lstrip("x"as), "helloxx"as);
		UTEST_EXPECT_EQ("xxhelloxx"as.rstrip("x"as), "xxhello"as);
		UTEST_EXPECT_EQ("one two two"as.replace("two"as, "2"as, 1), "one 2 two"as);
	};

	UTEST_SCOPE("测试 join、按分隔符 split、按空白 split 和 maxsplit。")
	{
		UTEST_EXPECT_EQ(","as.join(arr("我"as, "爱"as, "你"as)), "我,爱,你"as);
		UTEST_EXPECT_EQ(Atring::ajoin(arr("我"as, "爱"as, "你"as)), "我爱你"as);
		UTEST_EXPECT_EQ(Atring::ajoin(arr<AChar>('1', '2', '3', '4')), "1234"as);
		auto csv = "a,b,c"as.split(","as);
		UTEST_EXPECT_EQ(csv.size(), 3);
		UTEST_EXPECT_EQ(csv[0], "a"as);
		UTEST_EXPECT_EQ(csv[2], "c"as);
		auto limited = "a,b,c"as.split(","as, 1);
		UTEST_EXPECT_EQ(limited.size(), 2);
		UTEST_EXPECT_EQ(limited[1], "b,c"as);
		auto by_space = "a  b\tc"as.split();
		UTEST_EXPECT_EQ(by_space.size(), 3);
		UTEST_EXPECT_EQ(by_space[1], "b"as);
	};

	UTEST_SCOPE("测试大小写和字符分类。")
	{
		UTEST_EXPECT_EQ("AbC"as.lower(), "abc"as);
		UTEST_EXPECT_EQ("AbC"as.upper(), "ABC"as);
		UTEST_EXPECT("12345"as.isdigit());
		UTEST_EXPECT("abc"as.isalpha());
	};

    UTEST_SCOPE("测试整数判断与转换支持符号、前导零和不同进制。")
    {
        UTEST_EXPECT("0"as.isint());
        UTEST_EXPECT("+42"as.isint());
        UTEST_EXPECT("-42"as.isint());
        UTEST_EXPECT("00042"as.isint());
        UTEST_EXPECT("-101"as.isint(2));
        UTEST_EXPECT("755"as.isint(8));
        UTEST_EXPECT("aF"as.isint(16));
        UTEST_EXPECT("Zz"as.isint(36));
        UTEST_EXPECT(!"2"as.isint(2));
        UTEST_EXPECT(!"8"as.isint(8));
        UTEST_EXPECT(!"g"as.isint(16));
        UTEST_EXPECT(!"0x10"as.isint(16));

        UTEST_EXPECT_EQ("0"as.toint(), 0);
        UTEST_EXPECT_EQ("+42"as.toint(), 42);
        UTEST_EXPECT_EQ("-42"as.toint(), -42);
        UTEST_EXPECT_EQ("00042"as.toint(), 42);
        UTEST_EXPECT_EQ("-0"as.toint(), 0);
        UTEST_EXPECT_EQ("-101"as.toint(2), -5);
        UTEST_EXPECT_EQ("755"as.toint(8), 493);
        UTEST_EXPECT_EQ("aF"as.toint(16), 175);
        UTEST_EXPECT_EQ("+AF"as.toint(16), 175);
        UTEST_EXPECT_EQ("Zz"as.toint(36), 1295);
        UTEST_EXPECT_AYR_ERROR("2"as.toint(2));
        UTEST_EXPECT_AYR_ERROR("8"as.toint(8));
        UTEST_EXPECT_AYR_ERROR("g"as.toint(16));
        UTEST_EXPECT_AYR_ERROR("0x10"as.toint(16));
    };

    UTEST_SCOPE("测试整数判断与转换拒绝空文本、孤立符号及非整数内容。")
    {
        for (const Atring& text : {
            ""as, "+"as, "-"as, "++1"as, "--1"as, "+-1"as,
            "1-2"as, "1.0"as, "12kg"as, "1e3"as,
            " 1"as, "1 "as, "\t1"as, "1\n"as, "１２"as})
        {
            UTEST_EXPECT(!text.isint());
            UTEST_EXPECT_AYR_ERROR(text.toint());
        }
        for (int base : {-1, 0, 1, 37})
        {
            UTEST_EXPECT_AYR_ERROR("10"as.isint(base));
            UTEST_EXPECT_AYR_ERROR("10"as.toint(base));
        }
    };

    UTEST_SCOPE("测试浮点数和数字判断支持整数、小数、符号及不同进制。")
    {
        for (const Atring& text : {
            "0"as, "+42"as, "-42"as, "00042"as,
            "0.5"as, "+12.5"as, "-12.5"as, "1."as, "-0.0"as})
        {
            UTEST_EXPECT(text.isfloat(10));
            UTEST_EXPECT(text.isnum());
        }
        UTEST_EXPECT("10.1"as.isfloat(2));
        UTEST_EXPECT("a.F"as.isfloat(16));
        UTEST_EXPECT("Z.z"as.isnum(36));
        UTEST_EXPECT(!"10.2"as.isfloat(2));
        UTEST_EXPECT(!"a.G"as.isnum(16));
    };

    UTEST_SCOPE("测试浮点数和数字判断拒绝不完整数字及非法内容。")
    {
        for (const Atring& text : {
            ""as, "+"as, "-"as, "."as, "+."as, "-."as,
            ".5"as, "-.5"as, "1..0"as, "1.2.3"as, "1-2"as,
            "12.5kg"as, "1e3"as, "nan"as, "inf"as,
            " 1.5"as, "1.5 "as, "\t1.5"as, "1.5\n"as, "１２.５"as})
        {
            UTEST_EXPECT(!text.isfloat(10));
            UTEST_EXPECT(!text.isnum());
            UTEST_EXPECT_AYR_ERROR(text.tofloat());
        }
        for (int base : {-1, 0, 1, 37})
        {
            UTEST_EXPECT_AYR_ERROR("10.5"as.isfloat(base));
            UTEST_EXPECT_AYR_ERROR("10.5"as.isnum(base));
        }
    };

    UTEST_SCOPE("测试浮点转换返回完整数值并保留负零。")
    {
        UTEST_EXPECT_EQ("0"as.tofloat(), 0.0);
        UTEST_EXPECT_EQ("42"as.tofloat(), 42.0);
        UTEST_EXPECT_EQ("+42"as.tofloat(), 42.0);
        UTEST_EXPECT_EQ("-42"as.tofloat(), -42.0);
        UTEST_EXPECT_EQ("00012.5"as.tofloat(), 12.5);
        UTEST_EXPECT_EQ("+12.5"as.tofloat(), 12.5);
        UTEST_EXPECT_EQ("-12.5"as.tofloat(), -12.5);
        UTEST_EXPECT_EQ("1."as.tofloat(), 1.0);
        UTEST_EXPECT_EQ("-0.125"as.tofloat(), -0.125);

        double positive_zero = "+0.0"as.tofloat();
        double negative_zero = "-0.0"as.tofloat();
        UTEST_EXPECT_EQ(positive_zero, 0.0);
        UTEST_EXPECT(!std::signbit(positive_zero));
        UTEST_EXPECT_EQ(negative_zero, 0.0);
        UTEST_EXPECT(std::signbit(negative_zero));
        UTEST_EXPECT(std::signbit("-0"as.tofloat()));
    };

	Atring long_copy;
	UTEST_SCOPE("测试长字符串复制和切片独立于原对象的生命周期。")
	{
		{
			Atring source = "0123456789abcdef"as;
			long_copy = source;
			source = "short"as;
		}
		UTEST_EXPECT_EQ(long_copy, "0123456789abcdef"as);

		Atring long_slice = "0123456789abcdef"as.slice(2, 12);
		UTEST_EXPECT_EQ(long_slice, "23456789ab"as);
	};

	UTEST_SCOPE("测试长字符串移动后源对象回到有效的空 SSO 状态。")
	{
		Atring moved_long = std::move(long_copy);
		UTEST_EXPECT(long_copy.empty());
		UTEST_EXPECT_EQ(moved_long, "0123456789abcdef"as);

		Atring move_assigned_long;
		move_assigned_long = std::move(moved_long);
		UTEST_EXPECT(moved_long.empty());
		UTEST_EXPECT_EQ(move_assigned_long, "0123456789abcdef"as);
	};

	return UTEST_COMPLETE();
}
