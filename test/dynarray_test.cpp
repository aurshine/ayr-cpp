#include <algorithm>
#include <string>
#include <vector>

#include <ayr/air/DynArray.hpp>

#include <ayr/base/utest.hpp>

using namespace ayr;

int main()
{
	// 测试空数组、append、front/back、下标访问和跨 block 扩容。
	DynArray<int> values;
	AYR_TEST_EXPECT_EQ(values.size(), 0);
	for (int i = 0; i < 100; ++i)
		values.append(i);
	AYR_TEST_EXPECT_EQ(values.size(), 100);
	AYR_TEST_EXPECT_EQ(values.front(), 0);
	AYR_TEST_EXPECT_EQ(values.back(), 99);
	for (int i = 0; i < 100; ++i)
		AYR_TEST_EXPECT_EQ(values[i], i);

	// 测试插入到头部、中间和尾部后顺序正确。
	values.insert(0, -1);
	values.insert(50, 500);
	values.insert(values.size(), 1000);
	AYR_TEST_EXPECT_EQ(values.front(), -1);
	AYR_TEST_EXPECT_EQ(values[50], 500);
	AYR_TEST_EXPECT_EQ(values.back(), 1000);

	// 测试按下标删除、默认删除尾部和批量 pop_back。
	values.pop(50);
	AYR_TEST_EXPECT_EQ(values[50], 49);
	values.pop();
	AYR_TEST_EXPECT_EQ(values.back(), 99);
	values.pop_back(10);
	AYR_TEST_EXPECT_EQ(values.size(), 91);
	AYR_TEST_EXPECT_EQ(values.back(), 89);

	// 测试 extend、operator+、operator+= 和比较。
	DynArray<int> left{ 1, 2, 3 };
	DynArray<int> right{ 4, 5 };
	AYR_TEST_EXPECT_EQ(left + right, DynArray<int>({ 1, 2, 3, 4, 5 }));
	left += right;
	AYR_TEST_EXPECT_EQ(left, DynArray<int>({ 1, 2, 3, 4, 5 }));
	left.extend(range(3));
	AYR_TEST_EXPECT_EQ(left, DynArray<int>({ 1, 2, 3, 4, 5, 0, 1, 2 }));

	// 测试迭代器支持标准算法和距离计算。
	DynArray<int> unsorted{ 3, 1, 2 };
	std::sort(unsorted.begin(), unsorted.end());
	AYR_TEST_EXPECT_EQ(unsorted, DynArray<int>({ 1, 2, 3 }));
	AYR_TEST_EXPECT_EQ(unsorted.end() - unsorted.begin(), 3);

	// 测试拷贝、移动、to_array 和 move_array 的数据完整性。
	DynArray<std::string> words{ "a", "b", "c" };
	DynArray<std::string> copied(words);
	AYR_TEST_EXPECT_EQ(copied, words);
	DynArray<std::string> moved(std::move(copied));
	AYR_TEST_EXPECT_EQ(moved.size(), 3);
	auto arr = moved.to_array();
	AYR_TEST_EXPECT_EQ(arr.size(), 3);
	AYR_TEST_EXPECT_EQ(arr[1], "b");
	auto moved_arr = moved.move_array();
	AYR_TEST_EXPECT_EQ(moved_arr[2], "c");
	AYR_TEST_EXPECT_EQ(moved.size(), 0);

	// 测试 clear 后可以重新使用。
	values.clear();
	AYR_TEST_EXPECT_EQ(values.size(), 0);
	values.append(42);
	AYR_TEST_EXPECT_EQ(values.front(), 42);
}
