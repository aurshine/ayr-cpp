#ifndef AYR_BRANCHMARK_AIR_CONTAINER_BENCHMARK_HPP
#define AYR_BRANCHMARK_AIR_CONTAINER_BENCHMARK_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <ayr/base/Array.hpp>
#include <ayr/base/timer.hpp>

#include <ayr/base/meta.hpp>
#include "../report.hpp"

namespace container_benchmark
{
    constexpr std::size_t ITEM_COUNT = 1000'000;
    constexpr std::size_t SAMPLE_COUNT = 10;

    /** @brief 让标准容器使用与 AYR 相同的整数哈希，区分容器与哈希算法的收益。 */
    struct AyrIntegerHasher
    {
        std::size_t operator()(std::uint64_t value) const noexcept
        {
            return static_cast<std::size_t>(ayr::ayrhash(value));
        }
    };

    /** @brief 使用可逆混合生成互异的稀疏整数键，避免连续整数成为无碰撞的特例。 */
    inline std::uint64_t make_key(std::uint64_t value)
    {
        value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
        value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
        return value ^ (value >> 31);
    }

    /** @brief 在计时外准备固定种子的数据；offset 可生成不相交的未命中键。 */
    inline ayr::Array<uint64_t> make_keys(std::uint64_t offset = 0)
    {
        ayr::Array<uint64_t> keys(ITEM_COUNT);
        for (std::size_t i = 0; i < keys.size(); ++i)
            keys[i] = make_key(offset + i);
        std::mt19937 random(20261026);
        std::shuffle(keys.data(), keys.data() + keys.size(), random);
        return keys;
    }

    /** @brief 一轮操作的耗时和可观察结果，数组顺序与阶段名称一致。 */
    template<std::size_t N>
    struct Sample
    {
        ayr::Array<double> elapsed_ms = ayr::Array<double>(N);
        ayr::Array<uint64_t> checksums = ayr::Array<uint64_t>(N);
        static constexpr ayr::c_size phase_count = N;
    };

    /** @brief 仅计量操作本身，保留结果校验和，防止查询和遍历被优化掉。 */
    template<std::size_t N, typename F>
    void measure(Sample<N>& sample, std::size_t phase, F&& operation)
    {
        ayr::Timer_ms timer;
        timer.into();
        const std::uint64_t checksum = operation();
        sample.elapsed_ms[phase] = timer.escape();
        sample.checksums[phase] = checksum;
    }

    /** @brief 预热后交替执行两种容器，校验每轮结果，报告中位数但不断言耗时。 */
    template<typename AyrRun, typename StdRun>
    void compare(const ayr::CString& name, bool preallocated, const ayr::Array<ayr::CString>& phases,
        const ayr::Array<uint64_t>& expected, AyrRun&& run_ayr, StdRun&& run_std,
        const ayr::CString& workload = "random uint64 keys", const ayr::CString& allocation = "preallocated",
        const ayr::CString& details = "Data generation and initial allocation excluded; std max_load_factor = 0.75")
    {
        constexpr auto N = decltype(run_ayr())::phase_count;
        ayr::Array<Sample<N>> ayr_samples(SAMPLE_COUNT);
        ayr::Array<Sample<N>> std_samples(SAMPLE_COUNT);
        for (std::size_t round = 0; round <= SAMPLE_COUNT; ++round)
        {
            Sample<N> ayr_result, std_result;
            if (round % 2 == 0)
            {
                ayr_result = run_ayr();
                std_result = run_std();
            }
            else
            {
                std_result = run_std();
                ayr_result = run_ayr();
            }

            // 校验和断言均在所有阶段计时结束后执行，预热轮同样检查正确性。
            BENCHMARK_EXPECT(ayr_result.checksums == expected);
            BENCHMARK_EXPECT(std_result.checksums == expected);
            if (round != 0)
            {
                ayr_samples[round - 1] = ayr_result;
                std_samples[round - 1] = std_result;
            }
        }

        const auto title = ayr::fmt("{} / {} / {}", name, workload,
            preallocated ? allocation : ayr::CString("automatic growth"));
        const auto context = ayr::fmt("{} items; 1 warmup + {} samples. {}", ITEM_COUNT, SAMPLE_COUNT, details);
        benchmark_report::section(title, context);
        // Incorrect workloads must not produce apparently valid timing comparisons.
        if (benchmark_report::failures) return;
        for (std::size_t phase = 0; phase < N; ++phase)
        {
            ayr::Array<double> ayr_times(SAMPLE_COUNT), std_times(SAMPLE_COUNT);
            for (std::size_t i = 0; i < SAMPLE_COUNT; ++i)
            {
                ayr_times[i] = ayr_samples[i].elapsed_ms[phase];
                std_times[i] = std_samples[i].elapsed_ms[phase];
            }
            benchmark_report::comparison(phases[phase],
                benchmark_report::summarize(ayr_times), benchmark_report::summarize(std_times));
        }
    }
}

#endif // AYR_BRANCHMARK_AIR_CONTAINER_BENCHMARK_HPP
