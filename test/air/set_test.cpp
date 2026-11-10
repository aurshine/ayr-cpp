#include <array>
#include <cstdint>
#include <functional>
#include <unordered_set>
#include <vector>

#include <ayr/air/Set.hpp>

#include <ayr/base/utest.hpp>

#include "container_benchmark.hpp"

using namespace ayr;

namespace
{
    /** @brief 在相同数据和预分配策略下，对比 Set 与 unordered_set 的常用操作。 */
    template<typename StdHasher = std::hash<std::uint64_t>>
    void benchmark_set(bool preallocated, const char* name = "Set vs unordered_set")
    {
        using Key = std::uint64_t;
        using Result = container_benchmark::Sample<6>;
        const auto keys = container_benchmark::make_keys();
        const auto missing_keys = container_benchmark::make_keys(container_benchmark::ITEM_COUNT);
        std::uint64_t expected_sum = 0;
        for (Key key : keys) expected_sum += key;
        const std::array<const char*, 6> phases{
            "insert", "duplicate insert", "contains hit", "contains miss", "iterate", "erase"
        };
        const std::uint64_t count = keys.size();
        const std::array<std::uint64_t, 6> expected{ count, count, count, 0, expected_sum, 0 };

        const auto run_ayr = [&]()
        {
            // 初始分配在计时外；未预分配时，插入过程中的扩容计入耗时。
            Set<Key> values(preallocated ? static_cast<c_size>(keys.size()) : 0);
            Result result;
            container_benchmark::measure(result, 0, [&]()
            {
                for (Key key : keys) values.insert(key);
                return values.size();
            });
            container_benchmark::measure(result, 1, [&]()
            {
                for (Key key : keys) values.insert(key);
                return values.size();
            });
            container_benchmark::measure(result, 2, [&]()
            {
                std::uint64_t hits = 0;
                for (Key key : keys) hits += values.contains(key);
                return hits;
            });
            container_benchmark::measure(result, 3, [&]()
            {
                std::uint64_t hits = 0;
                for (Key key : missing_keys) hits += values.contains(key);
                return hits;
            });
            container_benchmark::measure(result, 4, [&]()
            {
                std::uint64_t sum = 0;
                for (Key key : values) sum += key;
                return sum;
            });
            container_benchmark::measure(result, 5, [&]()
            {
                for (Key key : keys) values.pop(key);
                return values.size();
            });
            return result;
        };
        const auto run_std = [&]()
        {
            std::unordered_set<Key, StdHasher> values;
            values.max_load_factor(0.75f);
            if (preallocated) values.reserve(keys.size());
            Result result;
            container_benchmark::measure(result, 0, [&]()
            {
                for (Key key : keys) values.insert(key);
                return values.size();
            });
            container_benchmark::measure(result, 1, [&]()
            {
                for (Key key : keys) values.insert(key);
                return values.size();
            });
            container_benchmark::measure(result, 2, [&]()
            {
                std::uint64_t hits = 0;
                for (Key key : keys) hits += values.contains(key);
                return hits;
            });
            container_benchmark::measure(result, 3, [&]()
            {
                std::uint64_t hits = 0;
                for (Key key : missing_keys) hits += values.contains(key);
                return hits;
            });
            container_benchmark::measure(result, 4, [&]()
            {
                std::uint64_t sum = 0;
                for (Key key : values) sum += key;
                return sum;
            });
            container_benchmark::measure(result, 5, [&]()
            {
                for (Key key : keys) values.erase(key);
                return values.size();
            });
            return result;
        };
        container_benchmark::compare(name, preallocated, phases, expected, run_ayr, run_std);
    }
}

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

    UTEST_SCOPE("对比 Set 与 unordered_set 自动扩容时的插入、去重、查询、遍历和删除性能。")
    {
        benchmark_set(false);
    };

    UTEST_SCOPE("对比 Set 与 unordered_set 提前预分配后的常用操作性能。")
    {
        benchmark_set(true);
    };

    UTEST_SCOPE("使用相同整数哈希，对比 Set 与 unordered_set 预分配后的容器性能。")
    {
        benchmark_set<container_benchmark::AyrIntegerHasher>(true, "Set vs unordered_set (same hash)");
    };

	return UTEST_COMPLETE();
}
