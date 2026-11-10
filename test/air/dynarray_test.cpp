#include <algorithm>
#include <array>
#include <cstdint>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include <ayr/air/DynArray.hpp>

#include <ayr/base/utest.hpp>

#include "container_benchmark.hpp"

using namespace ayr;

namespace
{
	struct SmallObject
	{
		std::uint64_t value;

		std::uint64_t checksum() const { return value; }
	};

	struct ExpensiveCopyObject
	{
		std::array<std::uint64_t, 16> values;

		ExpensiveCopyObject(std::uint64_t seed)
		{
			for (std::uint64_t i = 0; i < values.size(); ++i)
				values[i] = seed + i;
		}

		ExpensiveCopyObject(const ExpensiveCopyObject& other)
		{
			for (std::uint64_t i = 0; i < values.size(); ++i)
				values[i] = other.values[i];
		}

		std::uint64_t checksum() const
		{
			std::uint64_t result = 0;
			for (std::uint64_t value : values)
				result += value;
			return result;
		}
	};

    /** @brief 对比追加、随机读写、遍历和尾部删除，区分自动扩容与已有容量复用。 */
    template<typename T>
    void benchmark_dynarray(const char* object_name, bool reuse_capacity)
    {
        using Result = container_benchmark::Sample<5>;
        constexpr std::size_t COUNT = container_benchmark::ITEM_COUNT;
        std::vector<T> initial, updated;
        initial.reserve(COUNT);
        updated.reserve(COUNT);
        std::uint64_t initial_sum = 0, updated_sum = 0;
        for (std::size_t i = 0; i < COUNT; ++i)
        {
            initial.emplace_back(i + 1);
            updated.emplace_back(i + COUNT + 1);
            initial_sum += initial.back().checksum();
            updated_sum += updated.back().checksum();
        }
        std::vector<c_size> indices(COUNT);
        std::iota(indices.begin(), indices.end(), c_size{0});
        std::mt19937 random(20261026);
        std::shuffle(indices.begin(), indices.end(), random);
        const std::array<const char*, 5> phases{
            "append", "random read", "random write", "iterate", "pop_back"
        };
        const std::array<std::uint64_t, 5> expected{ COUNT, initial_sum, COUNT, updated_sum, updated_sum };

        const auto run_ayr = [&]()
        {
            DynArray<T> values;
            // DynArray 没有 reserve；双方都在计时外填充并清空，以复用已分配容量。
            if (reuse_capacity)
            {
                for (const T& item : initial) values.append(item);
                values.clear();
            }
            Result result;
            container_benchmark::measure(result, 0, [&]()
            {
                for (const T& item : initial) values.append(item);
                return values.size();
            });
            container_benchmark::measure(result, 1, [&]()
            {
                // DynArray::at 与 vector::operator[] 均不做边界检查。
                std::uint64_t sum = 0;
                for (c_size index : indices) sum += values.at(index).checksum();
                return sum;
            });
            container_benchmark::measure(result, 2, [&]()
            {
                for (c_size index : indices) values.at(index) = updated[index];
                return values.size();
            });
            container_benchmark::measure(result, 3, [&]()
            {
                std::uint64_t sum = 0;
                for (const T& item : values) sum += item.checksum();
                return sum;
            });
            container_benchmark::measure(result, 4, [&]()
            {
                // 读取每个被删除元素，使平凡析构类型的删除阶段也保留可观察结果。
                std::uint64_t sum = 0;
                for (std::size_t i = 0; i < COUNT; ++i)
                {
                    sum += values.back().checksum();
                    values.pop_back();
                }
                return sum ^ static_cast<std::uint64_t>(values.size());
            });
            return result;
        };
        const auto run_std = [&]()
        {
            std::vector<T> values;
            if (reuse_capacity)
            {
                for (const T& item : initial) values.push_back(item);
                values.clear();
            }
            Result result;
            container_benchmark::measure(result, 0, [&]()
            {
                for (const T& item : initial) values.push_back(item);
                return values.size();
            });
            container_benchmark::measure(result, 1, [&]()
            {
                std::uint64_t sum = 0;
                for (c_size index : indices) sum += values[index].checksum();
                return sum;
            });
            container_benchmark::measure(result, 2, [&]()
            {
                for (c_size index : indices) values[index] = updated[index];
                return values.size();
            });
            container_benchmark::measure(result, 3, [&]()
            {
                std::uint64_t sum = 0;
                for (const T& item : values) sum += item.checksum();
                return sum;
            });
            container_benchmark::measure(result, 4, [&]()
            {
                std::uint64_t sum = 0;
                for (std::size_t i = 0; i < COUNT; ++i)
                {
                    sum += values.back().checksum();
                    values.pop_back();
                }
                return sum ^ static_cast<std::uint64_t>(values.size());
            });
            return result;
        };
        container_benchmark::compare("DynArray vs vector", reuse_capacity, phases, expected, run_ayr, run_std,
            object_name, "reuse capacity after fill + clear",
            "Data generation and fill + clear setup excluded; random access is unchecked in both containers");
    }
}

int main()
{
	UTEST_SCOPE("测试 Appender 清空和同容量重置后复用内存，复制时保留容量。")
	{
		Appender<std::string> appender(8);
		appender.append("a");
		appender.append("b");
		auto* appender_data = appender.data();
		appender.clear();
		UTEST_EXPECT_EQ(appender.size(), 0);
		UTEST_EXPECT_EQ(appender.capacity(), 8);
		UTEST_EXPECT(appender.data() == appender_data);
		appender.resize(8);
		UTEST_EXPECT(appender.data() == appender_data);
		appender.append("copied");
		Appender<std::string> copied_appender(appender);
		UTEST_EXPECT_EQ(copied_appender.size(), 1);
		UTEST_EXPECT_EQ(copied_appender.capacity(), 8);
		UTEST_EXPECT_EQ(copied_appender[0], "copied");
	};

	DynArray<int> values;
	UTEST_SCOPE("测试空数组、append、front/back、下标访问和跨 block 扩容。")
	{
		UTEST_EXPECT_EQ(values.size(), 0);
		for (int i = 0; i < 100; ++i)
			values.append(i);
		UTEST_EXPECT_EQ(values.size(), 100);
		UTEST_EXPECT_EQ(values.front(), 0);
		UTEST_EXPECT_EQ(values.back(), 99);
		for (int i = 0; i < 100; ++i)
			UTEST_EXPECT_EQ(values[i], i);
	};

	UTEST_SCOPE("测试插入到头部、中间和尾部后顺序正确。")
	{
		values.insert(0, -1);
		values.insert(50, 500);
		values.insert(values.size(), 1000);
		UTEST_EXPECT_EQ(values.front(), -1);
		UTEST_EXPECT_EQ(values[50], 500);
		UTEST_EXPECT_EQ(values.back(), 1000);
	};

	UTEST_SCOPE("测试按下标删除、默认删除尾部和批量 pop_back。")
	{
		values.pop(50);
		UTEST_EXPECT_EQ(values[50], 49);
		values.pop();
		UTEST_EXPECT_EQ(values.back(), 99);
		values.pop_back(10);
		UTEST_EXPECT_EQ(values.size(), 91);
		UTEST_EXPECT_EQ(values.back(), 89);
	};

	UTEST_SCOPE("测试 extend、operator+、operator+= 和比较。")
	{
		DynArray<int> left{ 1, 2, 3 };
		DynArray<int> right{ 4, 5 };
		UTEST_EXPECT_EQ(left + right, DynArray<int>({ 1, 2, 3, 4, 5 }));
		left += right;
		UTEST_EXPECT_EQ(left, DynArray<int>({ 1, 2, 3, 4, 5 }));
		left.extend(range(3));
		UTEST_EXPECT_EQ(left, DynArray<int>({ 1, 2, 3, 4, 5, 0, 1, 2 }));
	};

	UTEST_SCOPE("测试迭代器支持标准算法和距离计算。")
	{
		DynArray<int> unsorted{ 3, 1, 2 };
		std::sort(unsorted.begin(), unsorted.end());
		UTEST_EXPECT_EQ(unsorted, DynArray<int>({ 1, 2, 3 }));
		UTEST_EXPECT_EQ(unsorted.end() - unsorted.begin(), 3);
	};

	UTEST_SCOPE("测试拷贝、移动、to_array 和 move_array 的数据完整性。")
	{
		DynArray<std::string> words{ "a", "b", "c" };
		DynArray<std::string> copied(words);
		UTEST_EXPECT_EQ(copied, words);
		DynArray<std::string> moved(std::move(copied));
		UTEST_EXPECT_EQ(moved.size(), 3);
		auto arr = moved.to_array();
		UTEST_EXPECT_EQ(arr.size(), 3);
		UTEST_EXPECT_EQ(arr[1], "b");
		auto moved_arr = moved.move_array();
		UTEST_EXPECT_EQ(moved_arr[2], "c");
		UTEST_EXPECT_EQ(moved.size(), 0);
	};

	UTEST_SCOPE("测试 clear 后可以重新使用。")
	{
		values.clear();
		UTEST_EXPECT_EQ(values.size(), 0);
		values.append(42);
		UTEST_EXPECT_EQ(values.front(), 42);
		UTEST_EXPECT_EQ(values.contains(42), true);
		UTEST_EXPECT_EQ(*values.find_it(42), 42);
		const DynArray<int>& const_values = values;
		UTEST_EXPECT_EQ(const_values.contains(42), true);
		UTEST_EXPECT_EQ(*const_values.find_it(42), 42);
	};

	UTEST_SCOPE("测试后置迭代操作修改当前迭代器并返回修改前的值。")
	{
		auto it = values.begin();
		auto old_it = it++;
		UTEST_EXPECT_EQ(*old_it, 42);
		UTEST_EXPECT(it == values.end());
	};

	UTEST_SCOPE("对比 DynArray 与 vector 处理十万个小对象时的常用操作性能。")
	{
        benchmark_dynarray<SmallObject>("small objects (8 bytes)", false);
        benchmark_dynarray<SmallObject>("small objects (8 bytes)", true);
	};

	UTEST_SCOPE("对比 DynArray 与 vector 处理十万个拷贝昂贵对象时的常用操作性能。")
	{
        benchmark_dynarray<ExpensiveCopyObject>("expensive-copy objects (128 bytes)", false);
        benchmark_dynarray<ExpensiveCopyObject>("expensive-copy objects (128 bytes)", true);
	};

	return UTEST_COMPLETE();
}
