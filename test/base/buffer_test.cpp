#include <cmath>
#include <cstdint>
#include <limits>

#include <ayr/base/utest.hpp>
#include <ayr/base/meta.hpp>

using namespace ayr;

namespace
{
    struct StringViewValue
    {
        std::string_view __str__() const { return std::string_view("a\0b", 3); }
    };

    struct CStringValue
    {
        CString __str__() const { return vstr("CString value"); }
    };

    template<typename T>
    bool float_round_trips(T value)
    {
        Buffer buffer(1);
        buffer << value;
        T parsed{};
        auto result = std::from_chars(buffer.peek(), buffer.peek() + buffer.readable_size(), parsed);
        return result.ec == std::errc{}
            && result.ptr == buffer.peek() + buffer.readable_size()
            && parsed == value
            && std::signbit(parsed) == std::signbit(value);
    }

    template<typename T>
    void check_float_boundaries()
    {
        UTEST_EXPECT(float_round_trips(T(0)));
        UTEST_EXPECT(float_round_trips(-T(0)));
        UTEST_EXPECT(float_round_trips(T(0.1L)));
        UTEST_EXPECT(float_round_trips(std::numeric_limits<T>::min()));
        UTEST_EXPECT(float_round_trips(std::numeric_limits<T>::denorm_min()));
        UTEST_EXPECT(float_round_trips(std::numeric_limits<T>::max()));
        UTEST_EXPECT(float_round_trips(std::numeric_limits<T>::lowest()));

        Buffer buffer;
        buffer << std::numeric_limits<T>::infinity() << ' '
            << -std::numeric_limits<T>::infinity() << ' '
            << std::numeric_limits<T>::quiet_NaN();
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), "inf -inf nan");
    }
}

int main()
{
    UTEST_SCOPE("Direct numeric writes append in arbitrary bases after compaction")
    {
        Buffer buffer(128);
        buffer << (CString("x") * 120);
        buffer.retrieve(119);
        char* allocation = buffer.data();
        buffer.append_number(std::numeric_limits<uint64_t>::max(), 2);
        UTEST_EXPECT(buffer.data() == allocation);
        UTEST_EXPECT(buffer.peek() == buffer.data());
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), cstr(CString("x") + (CString("1") * 64)));
        buffer.clear();
        buffer.append_number(-255, 16);
        buffer << ' ';
        buffer.append_number(35, 36);
        buffer << ' ';
        buffer.append_number(1.2f, 3);
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), "-ff z 1.200");
    };

    UTEST_SCOPE("Empty tuples and custom string representations stream recursively")
    {
        Buffer buffer(0);
        buffer << std::tuple<>{} << ' ' << std::tuple<int>{7} << ' '
            << std::tuple<int, std::tuple<>>{1, {}} << ' ' << CStringValue{} << ' ' << StringViewValue{};
        const CString expected("() (7) (1, ()) CString value a\0b", sizeof("() (7) (1, ()) CString value a\0b") - 1);
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()),
            expected);
        buffer.close();
        buffer.append_point(&buffer);
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()),
            expected);
    };

    UTEST_SCOPE("Buffer owns numeric writes and ignores writes after close")
    {
        Buffer buffer(0);
        UTEST_EXPECT(!buffer.closed());
        buffer.append_number(255, 16);
        buffer << ' ';
        buffer.append_number(1.25, 2);
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), "ff 1.25");
        buffer.close();
        const char* end = buffer.write_ptr();
        buffer.append_number(42);
        buffer.append_number(1.25, 1000);
        buffer.append_bytes("ignored", 7);
        buffer.adjust_util(1000);
        buffer.written(1);
        UTEST_EXPECT(buffer.write_ptr() == end);
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), "ff 1.25");
    };
    UTEST_SCOPE("Integer ranges and existing character/bool output")
    {
        Buffer buffer(1);
        buffer << std::numeric_limits<int64_t>::min() << ' '
            << std::numeric_limits<int64_t>::max() << ' '
            << std::numeric_limits<uint64_t>::max() << ' '
            << int8_t(-128) << ' ' << uint8_t(255) << ' '
            << int16_t(-32768) << ' ' << uint16_t(65535) << ' '
            << int32_t(-2147483647 - 1) << ' ' << uint32_t(4294967295U) << ' '
            << 0 << ' ' << true << ' ' << false << ' ' << 'x';
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()),
            "-9223372036854775808 9223372036854775807 18446744073709551615 "
            "-128 255 -32768 65535 -2147483648 4294967295 0 true false x");
    };

    UTEST_SCOPE("Shortest floating output and exact round trips")
    {
        check_float_boundaries<float>();
        check_float_boundaries<double>();
        check_float_boundaries<long double>();
        UTEST_EXPECT(float_round_trips(1e100));
        UTEST_EXPECT_EQ(cstr(0.1f), "0.1");
        UTEST_EXPECT_EQ(cstr(1.25), "1.25");
        UTEST_EXPECT_EQ(fmt("{} {}", UINT64_MAX, 0.0000001), "18446744073709551615 1e-07");
    };

    UTEST_SCOPE("Explicit decimal precision")
    {
        Buffer buffer(1);
        buffer << Fixed{1.234, 2} << ' ' << Fixed{1.2, 2} << ' '
            << Fixed{1.999, 2} << ' ' << Fixed{1.6, 0} << ' '
            << Fixed{-0.0, 2} << ' ' << Fixed{1.25, -1};
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), "1.23 1.20 2.00 2 -0.00 1.25");

        buffer.clear();
        buffer << Fixed{1.0, 1000};
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), CString("1.") + (CString("0") * 1000));

        buffer.clear();
        buffer << Fixed{std::numeric_limits<double>::max(), 2};
        UTEST_EXPECT_EQ(buffer.readable_size(), 312);
        UTEST_EXPECT(vstr(buffer.peek(), buffer.readable_size()).endswith(".00"));

        buffer.clear();
        buffer << Fixed{std::numeric_limits<double>::infinity(), 2};
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), "inf");
        buffer.close();
        buffer << Fixed{1.25, 2};
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), "inf");
    };

    UTEST_SCOPE("Pointer formatting uses a hexadecimal address with 0x prefix")
    {
        int value = 42;
        for (const void* pointer : {static_cast<const void*>(&value), static_cast<const void*>(nullptr)})
        {
            char expected[sizeof(void*) * 2 + 8];
            expected[0] = '0';
            expected[1] = 'x';
            auto result = std::to_chars(expected + 2, expected + sizeof(expected),
                reinterpret_cast<std::uintptr_t>(pointer), 16);
            Buffer buffer(1);
            buffer << pointer;
            UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), vstr(expected, result.ptr - expected));
        }
        UTEST_EXPECT_EQ(fmt("{}", static_cast<const void*>(nullptr)), "0x0");
        UTEST_EXPECT_EQ(fmt("{}", &value), fmt("{}", static_cast<const void*>(&value)));
    };

    UTEST_SCOPE("Appending after consumption, growth, and close")
    {
        Buffer buffer(64);
        buffer << (CString("x") * 60);
        buffer.retrieve(59);
        buffer << UINT64_MAX << ' ' << 1.25;
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), "x18446744073709551615 1.25");
        buffer << (CString("y") * 128) << ' ' << 1e100;
        UTEST_EXPECT(vstr(buffer.peek(), buffer.readable_size()).endswith(" 1e+100"));
        CString before(buffer.peek(), buffer.readable_size());
        buffer.close();
        buffer << 42 << 1e100 << static_cast<const void*>(&buffer);
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), before);
    };

    return UTEST_COMPLETE();
}
