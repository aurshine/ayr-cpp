#include <cstdint>
#include <functional>
#include <unordered_map>

#include <ayr/air/Dict.hpp>

#include <ayr/base/meta.hpp>
#include "../report.hpp"

#include "container_benchmark.hpp"

using namespace ayr;

namespace
{
    /** @brief 在相同数据和预分配策略下，对比 Dict 与 unordered_map 的常用操作。 */
    template<typename StdHasher = std::hash<std::uint64_t>>
    void benchmark_dict(bool preallocated, const CString& name = "Dict vs unordered_map")
    {
        using Key = std::uint64_t;
        using Val = std::uint64_t;
        using Result = container_benchmark::Sample<7>;
        const auto keys = container_benchmark::make_keys();
        const auto missing_keys = container_benchmark::make_keys(container_benchmark::ITEM_COUNT);
        std::uint64_t expected_sum = 0;
        for (Key key : keys)
            expected_sum += key ^ 0xa5a5a5a5a5a5a5a5ULL;
        const Array<CString> phases{
            "insert", "overwrite", "contains hit", "contains miss", "get value", "iterate", "erase"
        };
        const std::uint64_t count = keys.size();
        const Array<uint64_t> expected{
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
    benchmark_dict(false);
    benchmark_dict(true);
    benchmark_dict<container_benchmark::AyrIntegerHasher>(true, "Dict vs unordered_map (same hash)");
    return benchmark_report::complete();
}
