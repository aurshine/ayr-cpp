#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <limits>
#include <random>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include <format>

#include <ayr/base/meta/fmt.hpp>
#include <ayr/base/utest.hpp>

using namespace ayr;

namespace
{
    // MSVC 19.44 rejects implicit consteval construction with a non-trivial destructor.
    static_assert(std::is_trivially_destructible_v<CheckedFmtStr<1>>);

    struct StrValue
    {
        CString __str__() const { return vstr("custom string"); }
    };

    struct ReprValue
    {
        void __repr__(Buffer& buffer) const { buffer << "custom repr"; }
    };

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
        std::string text;
    };

    struct Sample
    {
        double milliseconds;
        uint64_t checksum;
    };

    /** @brief 计时包含结果分配、销毁和轻量校验和，不包含输入准备及断言。 */
    template<typename Format>
    Sample measure(const std::vector<Input>& inputs, Format format)
    {
        uint64_t checksum = 0;
        const auto start = std::chrono::steady_clock::now();
        for (size_t i = 0; i < ITERATIONS; ++i)
        {
            auto output = format(inputs[i % inputs.size()]);
            std::string_view bytes(output.data(), output.size());
            checksum = checksum * 131 + bytes.size();
            if (!bytes.empty())
                checksum += static_cast<unsigned char>(bytes.front())
                    + static_cast<unsigned char>(bytes[bytes.size() / 2])
                    + static_cast<unsigned char>(bytes.back());
        }
        const auto stop = std::chrono::steady_clock::now();
        return {std::chrono::duration<double, std::milli>(stop - start).count(), checksum};
    }

    /** @brief 先逐字节比较，再预热并交替采样；仅报告速度，不断言耗时。 */
    template<typename AyrFormat, typename StdFormat>
    void benchmark(const char* name, const std::vector<Input>& inputs,
        AyrFormat ayr_format, StdFormat std_format)
    {
        bool equal = true;
        for (const auto& input : inputs)
        {
            auto actual = ayr_format(input);
            auto expected = std_format(input);
            equal = equal && std::string_view(actual.data(), actual.size()) == expected;
        }
        UTEST_EXPECT(equal);
        if (!equal) return;

        const auto warm_ayr = measure(inputs, ayr_format);
        const auto warm_std = measure(inputs, std_format);
        UTEST_EXPECT_EQ(warm_ayr.checksum, warm_std.checksum);

        std::array<double, SAMPLE_COUNT> ayr_times{}, std_times{};
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
            UTEST_EXPECT_EQ(ayr_sample.checksum, warm_std.checksum);
            UTEST_EXPECT_EQ(std_sample.checksum, warm_std.checksum);
        }

        std::sort(ayr_times.begin(), ayr_times.end());
        std::sort(std_times.begin(), std_times.end());
        const double ayr_ms = ayr_times[SAMPLE_COUNT / 2];
        const double std_ms = std_times[SAMPLE_COUNT / 2];
        std::printf("%-16s %12.3f %12.3f %12.2f\n", name, ayr_ms, std_ms,
            ayr_ms > 0.0 ? std_ms / ayr_ms : 0.0);
    }

    void compare_with_std_format()
    {
        std::mt19937_64 random(20261110);
        std::vector<Input> inputs;
        inputs.reserve(INPUT_COUNT);
        for (size_t i = 0; i < INPUT_COUNT; ++i)
        {
            int64_t integer = static_cast<int64_t>(random() >> 1);
            if (i % 2) integer = -integer;
            inputs.push_back({integer, random(), static_cast<double>(random() % 4096) / 8.0,
                i % 2 != 0, "item-" + std::to_string(i)});
        }

        std::printf("\n[ayr::fmt vs std::format] %zu inputs, %zu operations/sample\n", INPUT_COUNT, ITERATIONS);
        std::printf("1 warmup + %zu samples; median milliseconds; std / ayr > 1 means ayr faster\n", SAMPLE_COUNT);
#ifdef NDEBUG
        std::printf("NDEBUG enabled; use an optimized Release build for performance comparisons.\n");
#else
        std::printf("Debug build: timings are diagnostic; use Release to compare performance.\n");
#endif
        std::printf("Includes allocation/destruction and a lightweight checksum; input preparation excluded.\n");
        std::printf("%-16s %12s %12s %12s\n", "workload", "ayr (ms)", "std (ms)", "std / ayr");
        benchmark("integers", inputs,
            [](const Input& in) { return ayr::fmt("id={} total={}", in.integer, in.unsigned_integer); },
            [](const Input& in) { return std::format("id={} total={}", in.integer, in.unsigned_integer); });
        benchmark("mixed", inputs,
            [](const Input& in) { return ayr::fmt("id={} cost={} active={} name={}", in.integer,
                in.fraction, in.flag, std::string_view(in.text)); },
            [](const Input& in) { return std::format("id={} cost={} active={} name={}", in.integer,
                in.fraction, in.flag, std::string_view(in.text)); });

        for (size_t i = 0; i < inputs.size(); ++i)
            inputs[i].text.append(256 + i % 64, 'x');
        benchmark("long strings", inputs,
            [](const Input& in) { return ayr::fmt("payload=[{}]", std::string_view(in.text)); },
            [](const Input& in) { return std::format("payload=[{}]", std::string_view(in.text)); });
    }
#endif
}

int main()
{
    UTEST_SCOPE("Literal text, placeholders, and empty arguments")
    {
        UTEST_EXPECT(ayr::fmt("").empty());
        UTEST_EXPECT_EQ(ayr::fmt("literal text"), "literal text");
        UTEST_EXPECT_EQ(ayr::fmt("{}", "hello"), "hello");
        UTEST_EXPECT_EQ(ayr::fmt("{}{}{}", "a", "", "c"), "ac");
        UTEST_EXPECT_EQ(ayr::fmt("before {} / {} after", 12, 34), "before 12 / 34 after");
        UTEST_EXPECT_EQ(ayr::fmt("{}", "{}"), "{}");
        UTEST_EXPECT_EQ(ayr::fmt("{}", "\xe4\xbd\xa0\xe5\xa5\xbd"), "\xe4\xbd\xa0\xe5\xa5\xbd");
    };

    UTEST_SCOPE("Numeric types and custom representations")
    {
        UTEST_EXPECT_EQ(ayr::fmt("{} {} {}", std::numeric_limits<int64_t>::min(),
            std::numeric_limits<int64_t>::max(), std::numeric_limits<uint64_t>::max()),
            "-9223372036854775808 9223372036854775807 18446744073709551615");
        UTEST_EXPECT_EQ(ayr::fmt("{} {} {} {}", true, false, 'x', 0), "true false x 0");
        UTEST_EXPECT_EQ(ayr::fmt("{} {} {}", 1.25, 0.1f, 1e100), "1.25 0.1 1e+100");
        UTEST_EXPECT_EQ(ayr::fmt("{} / {}", StrValue{}, ReprValue{}), "custom string / custom repr");
    };

    UTEST_SCOPE("String types, embedded nulls, growth, and result ownership")
    {
        std::string text = "standard";
        UTEST_EXPECT_EQ(ayr::fmt("{} {} {}", text, std::string_view("view"), vstr("CString")),
            "standard view CString");
        std::string binary("a\0b", 3);
        CString output = ayr::fmt("[{}]", binary);
        UTEST_EXPECT_EQ(output.size(), 5);
        UTEST_EXPECT_EQ(std::string(output.data(), output.size()), std::string("[a\0b]", 5));
        std::string long_text(4096, 'x');
        output = ayr::fmt("<{}>", long_text);
        UTEST_EXPECT_EQ(std::string(output.data(), output.size()), "<" + long_text + ">");
        output = [] { std::string local(128, 'y'); return ayr::fmt("{}", local); }();
        UTEST_EXPECT_EQ(std::string(output.data(), output.size()), std::string(128, 'y'));
    };

    UTEST_SCOPE("fmt_to appends literals and consecutive calls without clearing existing bytes")
    {
        Buffer buffer;
        buffer << "prefix:";
        ayr::fmt_to(buffer, "");
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), "prefix:");
        ayr::fmt_to(buffer, "literal");
        ayr::fmt_to(buffer, " {}{}{}", "a", "", "c");
        ayr::fmt_to(buffer, " [{}]", 42);
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), "prefix:literal ac [42]");

        buffer.clear();
        ayr::fmt_to(buffer, "{}", "{}");
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), "{}");
    };

    UTEST_SCOPE("fmt_to matches fmt for numeric, string, and custom arguments")
    {
        Buffer buffer(1);
        constexpr CheckedFmtStr<9> pattern("{}|{}|{}|{}|{}|{}|{}|{}|{}");
        std::string text = "standard";
        ayr::fmt_to(buffer, pattern, std::numeric_limits<int64_t>::min(),
            std::numeric_limits<uint64_t>::max(), 1.25, true, 'x', text,
            vstr("CString"), StrValue{}, ReprValue{});
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()),
            "-9223372036854775808|18446744073709551615|1.25|true|x|standard|CString|custom string|custom repr");
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()),
            ayr::fmt(pattern, std::numeric_limits<int64_t>::min(),
                std::numeric_limits<uint64_t>::max(), 1.25, true, 'x', text,
                vstr("CString"), StrValue{}, ReprValue{}));

        buffer.clear();
        ayr::fmt_to(buffer, "{} {} {}", std::string_view("view"), 0.1f, 1e100);
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), "view 0.1 1e+100");
    };

    UTEST_SCOPE("fmt_to preserves embedded nulls in arguments and format literals")
    {
        Buffer buffer;
        std::string binary("a\0b", 3);
        ayr::fmt_to(buffer, "[{}]\0tail", binary);
        UTEST_EXPECT_EQ(std::string(buffer.peek(), buffer.readable_size()), std::string("[a\0b]\0tail", 10));
    };

    UTEST_SCOPE("fmt_to handles compaction, growth, and reusing an allocated buffer")
    {
        Buffer buffer(64);
        buffer << std::string(60, 'x');
        buffer.retrieve(59);
        ayr::fmt_to(buffer, "value={}!", std::numeric_limits<uint64_t>::max());
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), "xvalue=18446744073709551615!");
        UTEST_EXPECT_EQ(buffer.peek(), buffer.data());
        UTEST_EXPECT_EQ(buffer.capacity(), 64);

        std::string long_text(4096, 'y');
        ayr::fmt_to(buffer, "<{}>", long_text);
        UTEST_EXPECT_EQ(std::string(buffer.peek(), buffer.readable_size()),
            "xvalue=18446744073709551615!<" + long_text + ">");
        c_size capacity = buffer.capacity();
        char* allocation = buffer.data();
        buffer.clear();
        constexpr CheckedFmtStr<2> pattern("{}:{}");
        ayr::fmt_to(buffer, pattern, "first", 1);
        ayr::fmt_to(buffer, pattern, "second", 2);
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), "first:1second:2");
        UTEST_EXPECT_EQ(buffer.capacity(), capacity);
        UTEST_EXPECT(buffer.data() == allocation);
    };

    UTEST_SCOPE("fmt_to retains results after arguments expire and ignores a closed target")
    {
        Buffer buffer;
        {
            std::string local(128, 'z');
            ayr::fmt_to(buffer, "{}", std::string_view(local));
        }
        UTEST_EXPECT_EQ(std::string(buffer.peek(), buffer.readable_size()), std::string(128, 'z'));
        buffer.close();
        ayr::fmt_to(buffer, "literal {} {} {}", 42, 1.25, "text");
        UTEST_EXPECT_EQ(std::string(buffer.peek(), buffer.readable_size()), std::string(128, 'z'));
    };

#if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
    UTEST_SCOPE("fmt_to matches std::format_to when appending to an existing target")
    {
        Buffer actual;
        actual << "prefix:";
        std::string expected = "prefix:";
        for (int i = -32; i <= 32; ++i)
        {
            ayr::fmt_to(actual, "[{} {} {} {}]", i, i / 8.0, i % 2 == 0, std::string_view("text"));
            std::format_to(std::back_inserter(expected), "[{} {} {} {}]", i, i / 8.0,
                i % 2 == 0, std::string_view("text"));
        }
        UTEST_EXPECT_EQ(std::string(actual.peek(), actual.readable_size()), expected);
    };

    UTEST_SCOPE("Correctness and performance compared with std::format")
    {
        compare_with_std_format();
    };
#else
    std::printf("[SKIPPED] std::format comparison: this standard library does not provide it.\n");
#endif
    return UTEST_COMPLETE();
}
