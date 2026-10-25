#ifndef AYR_TEST_AIR_CONTAINER_BENCHMARK_HPP
#define AYR_TEST_AIR_CONTAINER_BENCHMARK_HPP

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>

#include <ayr/base/utest.hpp>

namespace container_benchmark
{
    using ayr::TestRecorder;

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
    inline std::vector<std::uint64_t> make_keys(std::uint64_t offset = 0)
    {
        std::vector<std::uint64_t> keys(ITEM_COUNT);
        for (std::size_t i = 0; i < keys.size(); ++i)
            keys[i] = make_key(offset + i);
        std::mt19937 random(20261026);
        std::shuffle(keys.begin(), keys.end(), random);
        return keys;
    }

    /** @brief 一轮操作的耗时和可观察结果，数组顺序与阶段名称一致。 */
    template<std::size_t N>
    struct Sample
    {
        std::array<double, N> elapsed_ms{};
        std::array<std::uint64_t, N> checksums{};
    };

    /** @brief 仅计量操作本身，保留结果校验和，防止查询和遍历被优化掉。 */
    template<std::size_t N, typename F>
    void measure(Sample<N>& sample, std::size_t phase, F&& operation)
    {
        const auto start = std::chrono::steady_clock::now();
        const std::uint64_t checksum = operation();
        const auto stop = std::chrono::steady_clock::now();
        sample.elapsed_ms[phase] = std::chrono::duration<double, std::milli>(stop - start).count();
        sample.checksums[phase] = checksum;
    }

    /** @brief 预热后交替执行两种容器，校验每轮结果，报告中位数但不断言耗时。 */
    template<std::size_t N, typename AyrRun, typename StdRun>
    void compare(const char* name, bool preallocated, const std::array<const char*, N>& phases,
        const std::array<std::uint64_t, N>& expected, AyrRun&& run_ayr, StdRun&& run_std,
        const char* workload = "random uint64 keys", const char* allocation = "preallocated",
        const char* details = "Data generation and initial allocation excluded; std max_load_factor = 0.75")
    {
        std::array<Sample<N>, SAMPLE_COUNT> ayr_samples{};
        std::array<Sample<N>, SAMPLE_COUNT> std_samples{};
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
            UTEST_EXPECT(ayr_result.checksums == expected);
            UTEST_EXPECT(std_result.checksums == expected);
            if (round != 0)
            {
                ayr_samples[round - 1] = ayr_result;
                std_samples[round - 1] = std_result;
            }
        }

        std::printf("\n[%s benchmark] %zu %s, %s\n", name, ITEM_COUNT, workload,
            preallocated ? allocation : "automatic growth");
        std::printf("1 warmup + %zu samples; median milliseconds; ratio = std / ayr (>1: ayr faster)\n",
            SAMPLE_COUNT);
        std::printf("%s\n", details);
        std::printf("%-22s %12s %12s %12s\n", "operation", "ayr (ms)", "std (ms)", "std / ayr");
        for (std::size_t phase = 0; phase < N; ++phase)
        {
            std::array<double, SAMPLE_COUNT> ayr_times{}, std_times{};
            for (std::size_t i = 0; i < SAMPLE_COUNT; ++i)
            {
                ayr_times[i] = ayr_samples[i].elapsed_ms[phase];
                std_times[i] = std_samples[i].elapsed_ms[phase];
            }
            std::sort(ayr_times.begin(), ayr_times.end());
            std::sort(std_times.begin(), std_times.end());
            const double ayr_ms = ayr_times[SAMPLE_COUNT / 2];
            const double std_ms = std_times[SAMPLE_COUNT / 2];
            if (ayr_ms > 0.0)
                std::printf("%-22s %12.3f %12.3f %12.2f\n", phases[phase], ayr_ms, std_ms, std_ms / ayr_ms);
            else
                std::printf("%-22s %12.3f %12.3f %12s\n", phases[phase], ayr_ms, std_ms, "n/a");
        }
    }
}

#endif // AYR_TEST_AIR_CONTAINER_BENCHMARK_HPP
