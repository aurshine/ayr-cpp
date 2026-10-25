#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

#include <ayr/air/Dict.hpp>

#include <ayr/base/utest.hpp>

#include "container_benchmark.hpp"

using namespace ayr;

using DictIS = Dict<int, CString>;

namespace
{
    /** @brief 在相同数据和预分配策略下，对比 Dict 与 unordered_map 的常用操作。 */
    template<typename StdHasher = std::hash<std::uint64_t>>
    void benchmark_dict(bool preallocated, const char* name = "Dict vs unordered_map")
    {
        using Key = std::uint64_t;
        using Val = std::uint64_t;
        using Result = container_benchmark::Sample<7>;
        const auto keys = container_benchmark::make_keys();
        const auto missing_keys = container_benchmark::make_keys(container_benchmark::ITEM_COUNT);
        std::uint64_t expected_sum = 0;
        for (Key key : keys)
            expected_sum += key ^ 0xa5a5a5a5a5a5a5a5ULL;
        const std::array<const char*, 7> phases{
            "insert", "overwrite", "contains hit", "contains miss", "get value", "iterate", "erase"
        };
        const std::uint64_t count = keys.size();
        const std::array<std::uint64_t, 7> expected{
            count, count, count, 0, expected_sum, expected_sum, 0
        };

        const auto run_ayr = [&]()
        {
            // 初始分配在计时外；未预分配时，插入过程中的扩容计入耗时。
            Dict<Key, Val> values(preallocated ? static_cast<c_size>(keys.size()) : 0);
            Result result;
            container_benchmark::measure(result, 0, [&]()
            {
                for (Key key : keys) values.insert(key, key);
                return values.size();
            });
            container_benchmark::measure(result, 1, [&]()
            {
                for (Key key : keys) values.insert(key, key ^ 0xa5a5a5a5a5a5a5a5ULL);
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
                for (Key key : keys) sum += values.get(key);
                return sum;
            });
            container_benchmark::measure(result, 5, [&]()
            {
                std::uint64_t sum = 0;
                for (const auto& [key, value] : values) sum += value;
                return sum;
            });
            container_benchmark::measure(result, 6, [&]()
            {
                for (Key key : keys) values.pop(key);
                return values.size();
            });
            return result;
        };
        const auto run_std = [&]()
        {
            std::unordered_map<Key, Val, StdHasher> values;
            values.max_load_factor(0.75f);
            if (preallocated) values.reserve(keys.size());
            Result result;
            container_benchmark::measure(result, 0, [&]()
            {
                for (Key key : keys) values.insert_or_assign(key, key);
                return values.size();
            });
            container_benchmark::measure(result, 1, [&]()
            {
                for (Key key : keys) values.insert_or_assign(key, key ^ 0xa5a5a5a5a5a5a5a5ULL);
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
                for (Key key : keys) sum += values.at(key);
                return sum;
            });
            container_benchmark::measure(result, 5, [&]()
            {
                std::uint64_t sum = 0;
                for (const auto& [key, value] : values) sum += value;
                return sum;
            });
            container_benchmark::measure(result, 6, [&]()
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

    UTEST_SCOPE("对比 Dict 与 unordered_map 自动扩容时的插入、覆盖、查询、遍历和删除性能。")
    {
        benchmark_dict(false);
    };

    UTEST_SCOPE("对比 Dict 与 unordered_map 提前预分配后的常用操作性能。")
    {
        benchmark_dict(true);
    };

    UTEST_SCOPE("使用相同整数哈希，对比 Dict 与 unordered_map 预分配后的容器性能。")
    {
        benchmark_dict<container_benchmark::AyrIntegerHasher>(true, "Dict vs unordered_map (same hash)");
    };

	return UTEST_COMPLETE();
}
