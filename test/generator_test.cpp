#include <vector>

#include <ayr/coro.hpp>

#include <ayr/base/utest.hpp>

using namespace ayr;

coro::Generator<int> descending_numbers(int n)
{
	for (int i = n; i > 0; --i)
		co_yield i;
	co_return n + 1;
}

coro::Generator<int> empty_generator()
{
	if (false)
		co_yield 1;
	co_return coro::finish;
}

struct MoveOnly
{
	MoveOnly(int value) : value(value) {}
	MoveOnly(const MoveOnly&) = delete;
	MoveOnly(MoveOnly&& other) noexcept : value(other.value) { other.value = -1; }
	MoveOnly& operator=(const MoveOnly&) = delete;
	MoveOnly& operator=(MoveOnly&& other) noexcept
	{
		value = other.value;
		other.value = -1;
		return *this;
	}

	int value;
};

coro::Generator<MoveOnly> move_only_values()
{
	co_yield MoveOnly(1);
	co_yield MoveOnly(2);
	co_return coro::finish;
}

int main()
{
	// 测试 co_yield 序列和 co_return 末尾值都会被迭代出来。
	std::vector<int> values;
	for (int value : descending_numbers(3))
		values.push_back(value);
	AYR_TEST_EXPECT_EQ(values.size(), 4);
	AYR_TEST_EXPECT_EQ(values[0], 3);
	AYR_TEST_EXPECT_EQ(values[1], 2);
	AYR_TEST_EXPECT_EQ(values[2], 1);
	AYR_TEST_EXPECT_EQ(values[3], 4);

	// 测试没有 yield/return 的生成器为空。
	int count = 0;
	for (int value : empty_generator())
	{
		(void)value;
		++count;
	}
	AYR_TEST_EXPECT_EQ(count, 0);

	// 测试生成器支持不可拷贝、仅可移动的类型。
	int sum = 0;
	for (auto& item : move_only_values())
		sum += item.value;
	AYR_TEST_EXPECT_EQ(sum, 3);
}
