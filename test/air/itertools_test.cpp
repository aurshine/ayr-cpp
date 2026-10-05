#include <ayr/air/DynArray.hpp>
#include <ayr/base.hpp>
#include <ayr/base/utest.hpp>

using namespace ayr;

int main()
{
	UTEST_SCOPE("测试 range 的单参数、步长、空区间和非法参数。")
	{
		UTEST_EXPECT_EQ(sum(range(5)), 10);
		DynArray<c_size> stepped;
		for (c_size value : range(1, 8, 3))
			stepped.append(value);
		UTEST_EXPECT_EQ(stepped.size(), 3);
		UTEST_EXPECT_EQ(stepped[0], 1);
		UTEST_EXPECT_EQ(stepped[1], 4);
		UTEST_EXPECT_EQ(stepped[2], 7);
		DynArray<c_size> descending;
		for (c_size value : range(5, 0, -2))
			descending.append(value);
		UTEST_EXPECT_EQ(descending.size(), 3);
		UTEST_EXPECT_EQ(descending[0], 5);
		UTEST_EXPECT_EQ(descending[2], 1);
		UTEST_EXPECT(range(0).empty());
		UTEST_EXPECT_AYR_ERROR(range(0, 3, 0));
		UTEST_EXPECT_AYR_ERROR(range(3, 0, 1));
		UTEST_EXPECT_AYR_ERROR(range(0, 3, -1));
	};

	UTEST_SCOPE("测试 RangeIterator 双向和后置迭代。")
	{
		RangeIterator it(2, 2);
		auto old = it++;
		UTEST_EXPECT_EQ(*old, 2);
		UTEST_EXPECT_EQ(*it, 4);
		old = it--;
		UTEST_EXPECT_EQ(*old, 4);
		UTEST_EXPECT_EQ(*it, 2);
		UTEST_EXPECT_EQ(*it.operator->(), 2);
	};

	UTEST_SCOPE("测试 enumerate 索引、引用修改及后置迭代。")
	{
		DynArray<int> values{ 4, 5, 6 };
		c_size expected_index = 0;
		for (auto [index, value] : enumerate(values))
		{
			UTEST_EXPECT_EQ(index, expected_index++);
			value += 10;
		}
		UTEST_EXPECT_EQ(values[0], 14);
		UTEST_EXPECT_EQ(values[2], 16);

		auto it = enumerate(values).begin();
		auto old = it++;
		auto [old_index, old_value] = *old;
		UTEST_EXPECT_EQ(old_index, 0);
		UTEST_EXPECT_EQ(old_value, 14);
	};

	UTEST_SCOPE("测试 zip 以最短序列为准并保持元素引用。")
	{
		DynArray<int> left{ 1, 2, 3 };
		Array<int> right{ 10, 20 };
		int count = 0;
		for (auto [a, b] : zip(left, right))
		{
			a += b;
			++count;
		}
		UTEST_EXPECT_EQ(count, 2);
		UTEST_EXPECT_EQ(left[0], 11);
		UTEST_EXPECT_EQ(left[1], 22);
		UTEST_EXPECT_EQ(left[2], 3);

		auto it = zip(left, right).begin();
		auto old = it++;
		auto [old_left, old_right] = *old;
		UTEST_EXPECT_EQ(old_left, 11);
		UTEST_EXPECT_EQ(old_right, 10);
	};

	UTEST_SCOPE("测试 sum/min/max/all/any 的成功、短路和空序列。")
	{
		DynArray<int> values{ 3, 1, 4, 2 };
		UTEST_EXPECT_EQ(sum(values), 10);
		UTEST_EXPECT_EQ(sum(values, 5), 15);
		UTEST_EXPECT_EQ(min(values), 1);
		UTEST_EXPECT_EQ(max(values), 4);
		DynArray<int> empty;
		UTEST_EXPECT_AYR_ERROR(min(empty));
		UTEST_EXPECT_AYR_ERROR(max(empty));

		Array<bool> truths{ true, true, false };
		UTEST_EXPECT(!all(truths));
		UTEST_EXPECT(any(truths));
		UTEST_EXPECT(!any(empty, [](int value) { return value > 0; }));
		UTEST_EXPECT(all(empty, [](int value) { return value > 0; }));
		UTEST_EXPECT(all(values, [](int value) { return value > 0; }));
		UTEST_EXPECT(any(values, [](int value) { return value % 2 == 0; }));
		UTEST_EXPECT(!all(values, [](int value) { return value < 4; }));
	};

	return UTEST_COMPLETE();
}
