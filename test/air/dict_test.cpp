#include <string>

#include <ayr/air/Dict.hpp>

#include <ayr/base/utest.hpp>

using namespace ayr;

using DictIS = Dict<int, CString>;

int main()
{
	DictIS d{ {1, "one"}, {2, "two"} };
	UTEST_SCOPE("测试初始化、查询、默认值查询和 contains。")
	{
		UTEST_EXPECT_EQ(d.size(), 2);
		UTEST_EXPECT(d.contains(1));
		UTEST_EXPECT(!d.contains(9));
		UTEST_EXPECT_EQ(d.get(1), "one");
		UTEST_EXPECT_EQ(d.get(9, vstr("fallback")), "fallback");
	};

	UTEST_SCOPE("测试 insert 覆盖已有 key，operator[] 自动创建默认值。")
	{
		d.insert(2, "TWO");
		UTEST_EXPECT_EQ(d.get(2), "TWO");
		d[3] = "three";
		UTEST_EXPECT_EQ(d.size(), 3);
		UTEST_EXPECT_EQ(d[3], "three");
	};

	UTEST_SCOPE("测试 setdefault 对已有 key 不覆盖，对新 key 写入默认值。")
	{
		d.setdefault(3, "changed");
		d.setdefault(4, "four");
		UTEST_EXPECT_EQ(d.get(3), "three");
		UTEST_EXPECT_EQ(d.get(4), "four");
	};

	UTEST_SCOPE("测试 keys、values、items 视图能遍历完整内容。")
	{
		int key_sum = 0;
		for (auto key : d.keys())
			key_sum += key;
		UTEST_EXPECT_EQ(key_sum, 10);
		int value_count = 0;
		for (auto& value : d.values())
			if (!value.empty())
				++value_count;
		UTEST_EXPECT_EQ(value_count, 4);
		int item_count = 0;
		for (auto& [key, value] : d.items())
		{
			UTEST_EXPECT(d.contains(key));
			UTEST_EXPECT_EQ(d.get(key), value);
			++item_count;
		}
		UTEST_EXPECT_EQ(item_count, 4);
	};

	UTEST_SCOPE("测试删除存在和不存在的 key。")
	{
		d.pop(1);
		d.pop(99);
		UTEST_EXPECT_EQ(d.size(), 3);
		UTEST_EXPECT(!d.contains(1));
	};

	UTEST_SCOPE("测试字典交集、并集和对称差，冲突 key 保留左侧值。")
	{
		DictIS a{ {1, "a1"}, {2, "a2"}, {3, "a3"} };
		DictIS b{ {3, "b3"}, {4, "b4"} };
		UTEST_EXPECT_EQ(a & b, DictIS({ {3, "a3"} }));
		UTEST_EXPECT_EQ(a | b, DictIS({ {1, "a1"}, {2, "a2"}, {3, "a3"}, {4, "b4"} }));
		UTEST_EXPECT_EQ(a ^ b, DictIS({ {1, "a1"}, {2, "a2"}, {4, "b4"} }));
		UTEST_EXPECT_EQ(a & a, a);
		UTEST_EXPECT_EQ(a ^ a, DictIS());
	};

	UTEST_SCOPE("测试复合赋值和 clear。")
	{
		DictIS c{ {1, "one"}, {2, "two"} };
		c |= DictIS({ {2, "ignored"}, {3, "three"} });
		UTEST_EXPECT_EQ(c, DictIS({ {1, "one"}, {2, "two"}, {3, "three"} }));
		c &= DictIS({ {2, "x"}, {3, "x"} });
		UTEST_EXPECT_EQ(c, DictIS({ {2, "two"}, {3, "three"} }));
		c ^= DictIS({ {3, "x"}, {4, "four"} });
		UTEST_EXPECT_EQ(c, DictIS({ {2, "two"}, {4, "four"} }));
		c.clear();
		UTEST_EXPECT(c.empty());
		UTEST_EXPECT_EQ(c.size(), 0);
	};

	return UTEST_COMPLETE();
}
