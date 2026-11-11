#include <cstdint>
#include <iterator>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
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
}

int main()
{
    UTEST_SCOPE("BaseInt supports default decimal, bases 2 through 36, and integer boundaries")
    {
        UTEST_EXPECT_EQ(ayr::fmt("{} {} {} {} {}", BaseInt{42}, baseint(0, 2),
            baseint(255, 16), baseint(-255, 16), baseint(35, 36)), "42 0 ff -ff z");
        UTEST_EXPECT_EQ(ayr::fmt("{} {}", baseint(std::numeric_limits<int64_t>::min(), 16),
            baseint(std::numeric_limits<uint64_t>::max(), 16)), "-8000000000000000 ffffffffffffffff");
        UTEST_EXPECT_EQ(ayr::fmt("{}", baseint(std::numeric_limits<uint64_t>::max(), 2)),
            cstr((CString("1") * 64)));
        for (int base = 2; base <= 36; ++base)
        {
            UTEST_EXPECT_EQ(ayr::fmt("{}", baseint(base, base)), "10");
            UTEST_EXPECT_EQ(ayr::fmt("{}", baseint(-base, base)), "-10");
        }
        UTEST_EXPECT_EQ(ayr::fmt("{} {} {}", baseint(int8_t(-128)),
            baseint(uint8_t(255)), baseint('A', 16)), "-128 255 41");
    };

    UTEST_SCOPE("Fixed wrappers preserve float types, precision, rounding, and special values")
    {
        UTEST_EXPECT_EQ(ayr::fmt("{} {} {}", Fixed{0.1f}, fixed(1.25), fixed(1.25L, -2)),
            "0.1 1.25 1.25");
        UTEST_EXPECT_EQ(ayr::fmt("{} {} {} {} {}", fixed(1.234, 2), fixed(1.2f, 3),
            fixed(1.999, 2), fixed(1.6L, 0), fixed(-0.0, 2)), "1.23 1.200 2.00 2 -0.00");
        UTEST_EXPECT_EQ(ayr::fmt("{}", fixed(1.0, 1000)), cstr(CString("1.") + (CString("0") * 1000)));
        UTEST_EXPECT_EQ(ayr::fmt("{} {} {}", fixed(std::numeric_limits<double>::infinity(), 2),
            fixed(-std::numeric_limits<double>::infinity(), 0),
            fixed(std::numeric_limits<double>::quiet_NaN(), 3)), "inf -inf nan");
    };

    UTEST_SCOPE("Pad alignment, custom fills, and widths that do not truncate")
    {
        UTEST_EXPECT_EQ(ayr::fmt("[{}] [{}] [{}]", Pad{"xy", 5},
            left_pad("xy", 5), center_pad("xy", 5)), "[   xy] [xy   ] [ xy  ]");
        UTEST_EXPECT_EQ(ayr::fmt("{}|{}|{}", left_pad("xy", 6, '.'),
            right_pad(42, 5, '0'), pad("xy", 6, Pad::Align::Center, '-')),
            "xy....|00042|--xy--");
        for (c_size width : {-1, 0, 1, 2})
        {
            UTEST_EXPECT_EQ(ayr::fmt("{}", left_pad("xy", width)), "xy");
            UTEST_EXPECT_EQ(ayr::fmt("{}", right_pad("xy", width)), "xy");
            UTEST_EXPECT_EQ(ayr::fmt("{}", center_pad("xy", width)), "xy");
        }
        UTEST_EXPECT_EQ(ayr::fmt("{}", center_pad("", 3, '.')), "...");
        UTEST_EXPECT_EQ(ayr::fmt("{}", center_pad("xy", 3, '.')), "xy.");
        UTEST_EXPECT_EQ(ayr::fmt("{}", right_pad("x", 4096, '.')), cstr((CString(".") * 4095) + "x"));
    };

    UTEST_SCOPE("Wrappers compose in fmt_to, own temporary text, and preserve binary padding")
    {
        auto padded = [] { return right_pad((CString("x") * 128), 130, '.'); }();
        UTEST_EXPECT_EQ(ayr::fmt("{}", padded), cstr(CString("..") + (CString("x") * 128)));
        Buffer buffer(1);
        buffer << "prefix:";
        ayr::fmt_to(buffer, "{}|{}|{}", right_pad(baseint(255, 16), 4, '0'),
            left_pad(fixed(1.25, 2), 6, '.'), center_pad(StrValue{}, 16, '_'));
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()),
            "prefix:00ff|1.25..|_custom string__");
        CString binary = ayr::fmt("{}", right_pad(vstr("a\0b", 3), 5, '\0'));
        UTEST_EXPECT_EQ(vstr(binary.data(), binary.size()), vstr("\0\0a\0b", 5));
        buffer.close();
        ayr::fmt_to(buffer, "{}{}{}", baseint(42, 2), fixed(1.25, 2), left_pad("x", 1000));
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()),
            "prefix:00ff|1.25..|_custom string__");
    };

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
        UTEST_EXPECT_EQ(vstr(output.data(), output.size()), vstr("[a\0b]", 5));
        CString long_text = CString("x") * 4096;
        output = ayr::fmt("<{}>", long_text);
        UTEST_EXPECT_EQ(vstr(output.data(), output.size()), CString("<") + long_text + ">");
        output = [] { CString local = CString("y") * 128; return ayr::fmt("{}", local); }();
        UTEST_EXPECT_EQ(vstr(output.data(), output.size()), (CString("y") * 128));
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
        CString text = "standard";
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
        CString binary("a\0b", 3);
        ayr::fmt_to(buffer, "[{}]\0tail", binary);
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), vstr("[a\0b]\0tail", 10));
    };

    UTEST_SCOPE("fmt_to handles compaction, growth, and reusing an allocated buffer")
    {
        Buffer buffer(64);
        buffer << (CString("x") * 60);
        buffer.retrieve(59);
        ayr::fmt_to(buffer, "value={}!", std::numeric_limits<uint64_t>::max());
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), "xvalue=18446744073709551615!");
        UTEST_EXPECT_EQ(buffer.peek(), buffer.data());
        UTEST_EXPECT_EQ(buffer.capacity(), 64);

        CString long_text = CString("y") * 4096;
        ayr::fmt_to(buffer, "<{}>", long_text);
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()),
            CString("xvalue=18446744073709551615!<") + long_text + ">");
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
            CString local = CString("z") * 128;
            ayr::fmt_to(buffer, "{}", local);
        }
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), (CString("z") * 128));
        buffer.close();
        ayr::fmt_to(buffer, "literal {} {} {}", 42, 1.25, "text");
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), (CString("z") * 128));
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
        UTEST_EXPECT_EQ(vstr(actual.peek(), actual.readable_size()), vstr(expected));
    };

#endif
    return UTEST_COMPLETE();
}
