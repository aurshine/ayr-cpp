#include <vector>

#include <ayr/air/Set.hpp>

#include <ayr/base/utest.hpp>

using namespace ayr;

int main()
{
	Set<int> s{ 1, 2, 3 };
	UTEST_SCOPE("测试初始化、去重插入、contains、empty 和 size。")
	{
		UTEST_EXPECT(!s.empty());
		UTEST_EXPECT_EQ(s.size(), 3);
		s.insert(3);
		s.insert(4);
		UTEST_EXPECT_EQ(s.size(), 4);
		UTEST_EXPECT(s.contains(4));
		UTEST_EXPECT(!s.contains(99));
	};

	UTEST_SCOPE("测试删除存在和不存在的元素。")
	{
		s.pop(2);
		s.pop(99);
		UTEST_EXPECT_EQ(s.size(), 3);
		UTEST_EXPECT(!s.contains(2));
	};

	Set<int> b{ 3, 4, 5 };
	UTEST_SCOPE("测试集合交集、并集、对称差，以及自操作边界。")
	{
		Set<int> a{ 1, 2, 3, 4 };
		UTEST_EXPECT_EQ(a & b, Set<int>({ 3, 4 }));
		UTEST_EXPECT_EQ(a | b, Set<int>({ 1, 2, 3, 4, 5 }));
		UTEST_EXPECT_EQ(a ^ b, Set<int>({ 1, 2, 5 }));
		UTEST_EXPECT_EQ(a & a, a);
		UTEST_EXPECT_EQ(a ^ a, Set<int>());
	};

	UTEST_SCOPE("测试复合赋值操作保持语义一致。")
	{
		Set<int> c{ 1, 2, 3, 4 };
		c &= b;
		UTEST_EXPECT_EQ(c, Set<int>({ 3, 4 }));
		c |= Set<int>({ 4, 5, 6 });
		UTEST_EXPECT_EQ(c, Set<int>({ 3, 4, 5, 6 }));
		c ^= Set<int>({ 4, 6, 7 });
		UTEST_EXPECT_EQ(c, Set<int>({ 3, 5, 7 }));
	};

	Set<int> from_array;
	UTEST_SCOPE("测试由可迭代对象构造集合，并验证插入顺序迭代不丢元素。")
	{
		Array<int> arr{ 1, 1, 2, 3, 3 };
		from_array = set<int>(arr);
		UTEST_EXPECT_EQ(from_array, Set<int>({ 1, 2, 3 }));
		int sum = 0;
		for (int value : from_array)
			sum += value;
		UTEST_EXPECT_EQ(sum, 6);
	};

	UTEST_SCOPE("测试 clear 后状态复位。")
	{
		from_array.clear();
		UTEST_EXPECT(from_array.empty());
		UTEST_EXPECT_EQ(from_array.size(), 0);
	};

	return UTEST_COMPLETE();
}
