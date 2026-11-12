#include <algorithm>
#include <cstdint>
#include <random>
#include <vector>

#include <ayr/air/DynArray.hpp>

#include <ayr/base/meta.hpp>
#include "../report.hpp"

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
        std::uint64_t values[16];

        ExpensiveCopyObject(std::uint64_t seed)
        {
            for (std::uint64_t i = 0; i < 16; ++i)
                values[i] = seed + i;
        }

        ExpensiveCopyObject(const ExpensiveCopyObject& other)
        {
            for (std::uint64_t i = 0; i < 16; ++i)
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
    void benchmark_dynarray(const CString& object_name, bool reuse_capacity)
    {
        using Result = container_benchmark::Sample<5>;
        constexpr std::size_t COUNT = container_benchmark::ITEM_COUNT;
        DynArray<T> initial, updated;
        std::uint64_t initial_sum = 0, updated_sum = 0;
        for (std::size_t i = 0; i < COUNT; ++i)
        {
            initial.append(T(i + 1));
            updated.append(T(i + COUNT + 1));
            initial_sum += initial.back().checksum();
            updated_sum += updated.back().checksum();
        }
        Array<c_size> indices(COUNT);
        for (c_size i = 0; i < COUNT; ++i) indices[i] = i;
        std::mt19937 random(20261026);
        std::shuffle(indices.data(), indices.data() + indices.size(), random);
        const Array<CString> phases{
            "append", "random read", "random write", "iterate", "pop_back"
        };
        const Array<uint64_t> expected{ COUNT, initial_sum, COUNT, updated_sum, updated_sum };

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
    benchmark_dynarray<SmallObject>("small objects (8 bytes)", false);
    benchmark_dynarray<SmallObject>("small objects (8 bytes)", true);
    benchmark_dynarray<ExpensiveCopyObject>("expensive-copy objects (128 bytes)", false);
    benchmark_dynarray<ExpensiveCopyObject>("expensive-copy objects (128 bytes)", true);
    return benchmark_report::complete();
}
