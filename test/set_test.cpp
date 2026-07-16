#include <vector>

#include <ayr/air/Set.hpp>

#include <ayr/base/utest.hpp>

using namespace ayr;

int main()
{
	// 测试初始化、去重插入、contains、empty 和 size。
	Set<int> s{ 1, 2, 3 };
	AYR_TEST_EXPECT(!s.empty());
	AYR_TEST_EXPECT_EQ(s.size(), 3);
	s.insert(3);
	s.insert(4);
	AYR_TEST_EXPECT_EQ(s.size(), 4);
	AYR_TEST_EXPECT(s.contains(4));
	AYR_TEST_EXPECT(!s.contains(99));

	// 测试删除存在和不存在的元素。
	s.pop(2);
	s.pop(99);
	AYR_TEST_EXPECT_EQ(s.size(), 3);
	AYR_TEST_EXPECT(!s.contains(2));

	// 测试集合交集、并集、对称差，以及自操作边界。
	Set<int> a{ 1, 2, 3, 4 };
	Set<int> b{ 3, 4, 5 };
	AYR_TEST_EXPECT_EQ(a & b, Set<int>({ 3, 4 }));
	AYR_TEST_EXPECT_EQ(a | b, Set<int>({ 1, 2, 3, 4, 5 }));
	AYR_TEST_EXPECT_EQ(a ^ b, Set<int>({ 1, 2, 5 }));
	AYR_TEST_EXPECT_EQ(a & a, a);
	AYR_TEST_EXPECT_EQ(a ^ a, Set<int>());

	// 测试复合赋值操作保持语义一致。
	Set<int> c{ 1, 2, 3, 4 };
	c &= b;
	AYR_TEST_EXPECT_EQ(c, Set<int>({ 3, 4 }));
	c |= Set<int>({ 4, 5, 6 });
	AYR_TEST_EXPECT_EQ(c, Set<int>({ 3, 4, 5, 6 }));
	c ^= Set<int>({ 4, 6, 7 });
	AYR_TEST_EXPECT_EQ(c, Set<int>({ 3, 5, 7 }));

	// 测试由可迭代对象构造集合，并验证插入顺序迭代不丢元素。
	Array<int> arr{ 1, 1, 2, 3, 3 };
	Set<int> from_array = set<int>(arr);
	AYR_TEST_EXPECT_EQ(from_array, Set<int>({ 1, 2, 3 }));
	int sum = 0;
	for (int value : from_array)
		sum += value;
	AYR_TEST_EXPECT_EQ(sum, 6);

	// 测试 clear 后状态复位。
	from_array.clear();
	AYR_TEST_EXPECT(from_array.empty());
	AYR_TEST_EXPECT_EQ(from_array.size(), 0);
}
