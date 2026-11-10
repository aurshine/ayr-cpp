#include <ayr/base/meta/Buffer.hpp>

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

#include <ayr/base/utest.hpp>

using namespace ayr;

namespace
{
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
        buffer << FixedFloat{1.234, 2} << ' ' << FixedFloat{1.2, 2} << ' '
            << FixedFloat{1.999, 2} << ' ' << FixedFloat{1.6, 0} << ' '
            << FixedFloat{-0.0, 2} << ' ' << FixedFloat{1.25, -1};
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), "1.23 1.20 2.00 2 -0.00 1.25");

        buffer.clear();
        buffer << FixedFloat{1.0, 1000};
        UTEST_EXPECT_EQ(std::string(buffer.peek(), buffer.readable_size()), "1." + std::string(1000, '0'));

        buffer.clear();
        buffer << FixedFloat{std::numeric_limits<double>::max(), 2};
        UTEST_EXPECT_EQ(buffer.readable_size(), 312);
        UTEST_EXPECT(vstr(buffer.peek(), buffer.readable_size()).endswith(".00"));

        buffer.clear();
        buffer << FixedFloat{std::numeric_limits<double>::infinity(), 2};
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), "inf");
        buffer.close();
        buffer << FixedFloat{1.25, 2};
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), "inf");
    };

    UTEST_SCOPE("Pointer formatting retains platform conventions")
    {
        int value = 42;
        for (const void* pointer : {static_cast<const void*>(&value), static_cast<const void*>(nullptr)})
        {
            char expected[sizeof(void*) * 2 + 8];
            std::snprintf(expected, sizeof(expected), "%p", const_cast<void*>(pointer));
            Buffer buffer(1);
            buffer << pointer;
            UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), expected);
        }
    };

    UTEST_SCOPE("Appending after consumption, growth, and close")
    {
        Buffer buffer(64);
        buffer << std::string(60, 'x');
        buffer.retrieve(59);
        buffer << UINT64_MAX << ' ' << 1.25;
        UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), "x18446744073709551615 1.25");
        buffer << std::string(128, 'y') << ' ' << 1e100;
        UTEST_EXPECT(vstr(buffer.peek(), buffer.readable_size()).endswith(" 1e+100"));
        std::string before(buffer.peek(), buffer.readable_size());
        buffer.close();
        buffer << 42 << 1e100 << static_cast<const void*>(&buffer);
        UTEST_EXPECT_EQ(std::string(buffer.peek(), buffer.readable_size()), before);
    };

    return UTEST_COMPLETE();
}
