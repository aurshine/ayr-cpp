#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <ayr/air/DynArray.hpp>

#include <ayr/base/utest.hpp>

using namespace ayr;

namespace
{
	constexpr c_size BENCHMARK_SIZE = 1'000'000;

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

	struct BenchmarkResult
	{
		double elapsed_ms;
		std::uint64_t checksum;
	};

	template<typename T>
	BenchmarkResult benchmark_dynarray(const T& sample)
	{
		Timer_ms timer;
		timer.into();

		DynArray<T> values;
		for (c_size i = 0; i < BENCHMARK_SIZE; ++i)
			values.append(sample);

		const double elapsed_ms = timer.escape();
		std::uint64_t checksum = 0;
		for (const T& value : values)
			checksum += value.checksum();
		return { elapsed_ms, checksum };
	}

	template<typename T>
	BenchmarkResult benchmark_vector(const T& sample, bool reserve)
	{
		Timer_ms timer;
		timer.into();

		std::vector<T> values;
		if (reserve)
			values.reserve(BENCHMARK_SIZE);
		for (c_size i = 0; i < BENCHMARK_SIZE; ++i)
			values.push_back(sample);

		const double elapsed_ms = timer.escape();
		std::uint64_t checksum = 0;
		for (const T& value : values)
			checksum += value.checksum();
		return { elapsed_ms, checksum };
	}

	template<typename T>
	void benchmark_append(const char* object_name, const T& sample)
	{
		const BenchmarkResult dynarray_result = benchmark_dynarray(sample);
		const BenchmarkResult vector_result = benchmark_vector(sample, false);
		const BenchmarkResult reserved_vector_result = benchmark_vector(sample, true);

		print("\n[DynArray benchmark]", object_name, BENCHMARK_SIZE, "items");
		print("DynArray         append:", dynarray_result.elapsed_ms, "ms");
		print("vector           append:", vector_result.elapsed_ms, "ms");
		print("vector reserve + append:", reserved_vector_result.elapsed_ms, "ms");

		UTEST_EXPECT_EQ(dynarray_result.checksum, vector_result.checksum);
		UTEST_EXPECT_EQ(dynarray_result.checksum, reserved_vector_result.checksum);
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

	UTEST_SCOPE("对比一百万个小对象对象的连续追加效率。")
	{
		benchmark_append("small object", SmallObject{ 7 });
	};

	UTEST_SCOPE("对比一百万个拷贝昂贵对象的连续追加效率。")
	{
		benchmark_append("expensive-copy object", ExpensiveCopyObject{ 7 });
	};

	return UTEST_COMPLETE();
}
