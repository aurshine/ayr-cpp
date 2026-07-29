#include <string>

#include <ayr/air/Dict.hpp>

#include <ayr/base/utest.hpp>

using namespace ayr;

using DictIS = Dict<int, CString>;

int main()
{
	// 测试初始化、查询、默认值查询和 contains。
	DictIS d{ {1, "one"}, {2, "two"} };
	AYR_TEST_EXPECT_EQ(d.size(), 2);
	AYR_TEST_EXPECT(d.contains(1));
	AYR_TEST_EXPECT(!d.contains(9));
	AYR_TEST_EXPECT_EQ(d.get(1), "one");
	AYR_TEST_EXPECT_EQ(d.get(9, vstr("fallback")), "fallback");

	// 测试 insert 覆盖已有 key，operator[] 自动创建默认值。
	d.insert(2, "TWO");
	AYR_TEST_EXPECT_EQ(d.get(2), "TWO");
	d[3] = "three";
	AYR_TEST_EXPECT_EQ(d.size(), 3);
	AYR_TEST_EXPECT_EQ(d[3], "three");

	// 测试 setdefault 对已有 key 不覆盖，对新 key 写入默认值。
	d.setdefault(3, "changed");
	d.setdefault(4, "four");
	AYR_TEST_EXPECT_EQ(d.get(3), "three");
	AYR_TEST_EXPECT_EQ(d.get(4), "four");

	// 测试 keys、values、items 视图能遍历完整内容。
	int key_sum = 0;
	for (auto key : d.keys())
		key_sum += key;
	AYR_TEST_EXPECT_EQ(key_sum, 10);
	int value_count = 0;
	for (auto& value : d.values())
		if (!value.empty())
			++value_count;
	AYR_TEST_EXPECT_EQ(value_count, 4);
	int item_count = 0;
	for (auto& [key, value] : d.items())
	{
		AYR_TEST_EXPECT(d.contains(key));
		AYR_TEST_EXPECT_EQ(d.get(key), value);
		++item_count;
	}
	AYR_TEST_EXPECT_EQ(item_count, 4);

	// 测试删除存在和不存在的 key。
	d.pop(1);
	d.pop(99);
	AYR_TEST_EXPECT_EQ(d.size(), 3);
	AYR_TEST_EXPECT(!d.contains(1));

	// 测试字典交集、并集和对称差，冲突 key 保留左侧值。
	DictIS a{ {1, "a1"}, {2, "a2"}, {3, "a3"} };
	DictIS b{ {3, "b3"}, {4, "b4"} };
	AYR_TEST_EXPECT_EQ(a & b, DictIS({ {3, "a3"} }));
	AYR_TEST_EXPECT_EQ(a | b, DictIS({ {1, "a1"}, {2, "a2"}, {3, "a3"}, {4, "b4"} }));
	AYR_TEST_EXPECT_EQ(a ^ b, DictIS({ {1, "a1"}, {2, "a2"}, {4, "b4"} }));
	AYR_TEST_EXPECT_EQ(a & a, a);
	AYR_TEST_EXPECT_EQ(a ^ a, DictIS());

	// 测试复合赋值和 clear。
	DictIS c{ {1, "one"}, {2, "two"} };
	c |= DictIS({ {2, "ignored"}, {3, "three"} });
	AYR_TEST_EXPECT_EQ(c, DictIS({ {1, "one"}, {2, "two"}, {3, "three"} }));
	c &= DictIS({ {2, "x"}, {3, "x"} });
	AYR_TEST_EXPECT_EQ(c, DictIS({ {2, "two"}, {3, "three"} }));
	c ^= DictIS({ {3, "x"}, {4, "four"} });
	AYR_TEST_EXPECT_EQ(c, DictIS({ {2, "two"}, {4, "four"} }));
	c.clear();
	AYR_TEST_EXPECT(c.empty());
	AYR_TEST_EXPECT_EQ(c.size(), 0);
}
