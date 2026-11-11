#include <algorithm>
#include <cstdint>
#include <random>
#include <string_view>
#include <format>

#include <ayr/base/meta/fmt.hpp>
#include <ayr/base/meta.hpp>
#include <ayr/air/DynArray.hpp>
#include <ayr/base/Array.hpp>
#include <ayr/base/timer.hpp>
#include "../report.hpp"

using namespace ayr;

namespace
{
#if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
    constexpr size_t INPUT_COUNT = 1024;
    constexpr size_t ITERATIONS = 100'000;
    constexpr size_t SAMPLE_COUNT = 7;

    struct Input
    {
        int64_t integer;
        uint64_t unsigned_integer;
        double fraction;
        bool flag;
        CString text;
    };

    struct Sample
    {
        double milliseconds;
        uint64_t checksum;
    };

    /** @brief 计时包含结果分配、销毁和轻量校验和，不包含输入准备及断言。 */
    template<typename Format>
    Sample measure(const DynArray<Input>& inputs, Format format)
    {
        uint64_t checksum = 0;
        Timer_ms timer;
        timer.into();
        for (size_t i = 0; i < ITERATIONS; ++i)
        {
            auto output = format(inputs[i % inputs.size()]);
            auto bytes = vstr(output.data(), output.size());
            checksum = checksum * 131 + bytes.size();
            if (!bytes.empty())
                checksum += static_cast<unsigned char>(bytes[0])
                    + static_cast<unsigned char>(bytes[bytes.size() / 2])
                    + static_cast<unsigned char>(bytes[bytes.size() - 1]);
        }
        return {timer.escape(), checksum};
    }

    /** @brief 先逐字节比较，再预热并交替采样；仅报告速度，不断言耗时。 */
    template<typename AyrFormat, typename StdFormat>
    void benchmark(const CString& name, const DynArray<Input>& inputs,
        AyrFormat ayr_format, StdFormat std_format)
    {
        bool equal = true;
        for (const auto& input : inputs)
        {
            auto actual = ayr_format(input);
            auto expected = std_format(input);
            equal = equal && vstr(actual.data(), actual.size()) == vstr(expected.data(), expected.size());
        }
        BENCHMARK_EXPECT(equal);
        if (!equal) return;

        const auto warm_ayr = measure(inputs, ayr_format);
        const auto warm_std = measure(inputs, std_format);
        BENCHMARK_EXPECT_EQ(warm_ayr.checksum, warm_std.checksum);

        Array<double> ayr_times(SAMPLE_COUNT), std_times(SAMPLE_COUNT);
        for (size_t i = 0; i < SAMPLE_COUNT; ++i)
        {
            Sample ayr_sample{}, std_sample{};
            if (i % 2 == 0)
            {
                ayr_sample = measure(inputs, ayr_format);
                std_sample = measure(inputs, std_format);
            }
            else
            {
                std_sample = measure(inputs, std_format);
                ayr_sample = measure(inputs, ayr_format);
            }
            ayr_times[i] = ayr_sample.milliseconds;
            std_times[i] = std_sample.milliseconds;
            BENCHMARK_EXPECT_EQ(ayr_sample.checksum, warm_std.checksum);
            BENCHMARK_EXPECT_EQ(std_sample.checksum, warm_std.checksum);
        }

        if (benchmark_report::failures) return;
        benchmark_report::comparison(name,
            benchmark_report::summarize(ayr_times), benchmark_report::summarize(std_times));
    }

    void compare_with_std_format()
    {
        std::mt19937_64 random(20261110);
        DynArray<Input> inputs;
        for (size_t i = 0; i < INPUT_COUNT; ++i)
        {
            int64_t integer = static_cast<int64_t>(random() >> 1);
            if (i % 2) integer = -integer;
            inputs.append(Input{integer, random(), static_cast<double>(random() % 4096) / 8.0,
                i % 2 != 0, ayr::fmt("item-{}", i)});
        }

        benchmark_report::section("ayr::fmt vs std::format",
            "1024 inputs; 100000 operations/sample; 1 warmup + 7 samples. Allocation/destruction and checksum included; input preparation excluded.");
#ifdef NDEBUG
        ayr::print("NDEBUG enabled; use an optimized Release build for performance comparisons.\n");
#else
        ayr::print("Debug build: timings are diagnostic; use Release to compare performance.\n");
#endif
        benchmark("integers", inputs,
            [](const Input& in) { return ayr::fmt("id={} total={}", in.integer, in.unsigned_integer); },
            [](const Input& in) { return std::format("id={} total={}", in.integer, in.unsigned_integer); });
        benchmark("mixed", inputs,
            [](const Input& in) { return ayr::fmt("id={} cost={} active={} name={}", in.integer,
                in.fraction, in.flag, in.text); },
            [](const Input& in) { return std::format("id={} cost={} active={} name={}", in.integer,
                in.fraction, in.flag, std::string_view(in.text.data(), in.text.size())); });

        for (size_t i = 0; i < inputs.size(); ++i)
            inputs[i].text += CString("x") * (256 + i % 64);
        benchmark("long strings", inputs,
            [](const Input& in) { return ayr::fmt("payload=[{}]", in.text); },
            [](const Input& in) { return std::format("payload=[{}]", std::string_view(in.text.data(), in.text.size())); });
    }
#endif
}

int main()
{
#if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
    compare_with_std_format();
#else
    ayr::print("[SKIPPED] std::format comparison: this standard library does not provide it.\n");
#endif
    return benchmark_report::complete();
}
