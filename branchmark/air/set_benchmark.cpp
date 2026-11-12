#include <cstdint>
#include <functional>
#include <unordered_set>

#include <ayr/air/Set.hpp>

#include <ayr/base/meta.hpp>
#include "../report.hpp"

#include "container_benchmark.hpp"

using namespace ayr;

namespace
{
    /** @brief 在相同数据和预分配策略下，对比 Set 与 unordered_set 的常用操作。 */
    template<typename StdHasher = std::hash<std::uint64_t>>
    void benchmark_set(bool preallocated, const CString& name = "Set vs unordered_set")
    {
        using Key = std::uint64_t;
        using Result = container_benchmark::Sample<6>;
        const auto keys = container_benchmark::make_keys();
        const auto missing_keys = container_benchmark::make_keys(container_benchmark::ITEM_COUNT);
        std::uint64_t expected_sum = 0;
        for (Key key : keys) expected_sum += key;
        const Array<CString> phases{
            "insert", "duplicate insert", "contains hit", "contains miss", "iterate", "erase"
        };
        const std::uint64_t count = keys.size();
        const Array<uint64_t> expected{ count, count, count, 0, expected_sum, 0 };

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
    benchmark_set(false);
    benchmark_set(true);
    benchmark_set<container_benchmark::AyrIntegerHasher>(true, "Set vs unordered_set (same hash)");
    return benchmark_report::complete();
}
