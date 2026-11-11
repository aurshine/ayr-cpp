#include <ayr/base/Array.hpp>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <string>

#include <ayr/base/meta/hash.hpp>
#include <ayr/base/utest.hpp>

namespace hash_cases
{
    enum class Id : std::int64_t { NEGATIVE = -7, POSITIVE = 42 };

    struct CustomKey
    {
        int value;
        ayr::hash_t __hash__() const { return 1234; }
        bool operator==(const CustomKey&) const = default;
    };

    struct DualKey
    {
        int value;
        ayr::hash_t __hash__() const { return 5678; }
        bool operator==(const DualKey&) const = default;
    };

    struct StandardKey
    {
        int value;
        bool operator==(const StandardKey&) const = default;
    };
}

namespace std
{
    template<>
    struct hash<hash_cases::DualKey>
    {
        size_t operator()(const hash_cases::DualKey&) const { return 9999; }
    };

    template<>
    struct hash<hash_cases::StandardKey>
    {
        size_t operator()(const hash_cases::StandardKey& key) const { return key.value + 100; }
    };
}

using namespace ayr;

int main()
{
    UTEST_SCOPE("整数哈希支持不同宽度、负数、枚举和完整的 64 位值。")
    {
        static_assert(integer_hash(0) == 0);
        static_assert(integer_hash(1) != integer_hash(2));
        UTEST_EXPECT_EQ(ayrhash(std::int8_t{-7}), ayrhash(std::int64_t{-7}));
        UTEST_EXPECT_EQ(ayrhash(std::uint16_t{42}), ayrhash(std::uint64_t{42}));
        UTEST_EXPECT_EQ(ayrhash(hash_cases::Id::NEGATIVE), ayrhash(std::int64_t{-7}));
        UTEST_EXPECT_EQ(ayrhash(hash_cases::Id::POSITIVE), ayrhash(42));
        UTEST_EXPECT(ayrhash(false) != ayrhash(true));
        UTEST_EXPECT(ayrhash(std::uint64_t{1}) != ayrhash(std::uint64_t{1} << 32));
        UTEST_EXPECT(ayrhash(std::numeric_limits<std::int64_t>::min())
            != ayrhash(std::numeric_limits<std::int64_t>::max()));
        UTEST_EXPECT(ayrhash(std::numeric_limits<std::uint64_t>::max()) != ayrhash(std::uint64_t{0}));
    };

    UTEST_SCOPE("对齐整数和仅高位变化的整数，在二次幂桶中仍具有充分的低位分布。")
    {
        constexpr std::size_t COUNT = 4096;
        constexpr std::size_t CAPACITY = 16384;
        for (int shift : {12, 32})
        {
            Array<unsigned> buckets(CAPACITY, 0);
            for (std::uint64_t i = 0; i < COUNT; ++i)
                ++buckets[ayrhash(i << shift) & (CAPACITY - 1)];
            std::size_t occupied = 0;
            unsigned maximum = 0;
            for (unsigned count : buckets)
            {
                occupied += count != 0;
                if (count > maximum) maximum = count;
            }
            UTEST_EXPECT(occupied > COUNT * 3 / 4);
            UTEST_EXPECT(maximum < 8);
        }
    };

    UTEST_SCOPE("自定义哈希优先于标准哈希，其余非整数类型保留标准行为。")
    {
        UTEST_EXPECT_EQ(ayrhash(hash_cases::CustomKey{1}), 1234);
        UTEST_EXPECT_EQ(ayrhash(hash_cases::DualKey{1}), 5678);
        UTEST_EXPECT_EQ(ayrhash(hash_cases::StandardKey{7}), 107);
        UTEST_EXPECT_EQ(ayrhash(0.0), ayrhash(-0.0));
        UTEST_EXPECT_EQ(ayrhash(1.5), std::hash<double>{}(1.5));
        const std::string key = "standard string";
        UTEST_EXPECT_EQ(ayrhash(key), std::hash<std::string>{}(key));
        int value = 7;
        UTEST_EXPECT_EQ(ayrhash(&value), std::hash<int*>{}(&value));
    };

    UTEST_SCOPE("字节哈希保持优化前的二进制输入和种子结果，并支持未对齐读取。")
    {
        struct Fixture { std::size_t length; hash_t standard_seed; hash_t custom_seed; };
        // 在替换实现前保存的结果，包含高位字节及恰好四字节的尾部。
        constexpr Fixture FIXTURES[] = {
            {0, 0x925c616f54db4dd4ULL, 0x000003bd85ac7f13ULL},
            {1, 0x5ed8d157529dd694ULL, 0x9a231ab3dbf4cc74ULL},
            {2, 0xed51174499fa6b47ULL, 0x3446d6476b6bde78ULL},
            {3, 0x47271da322c9c535ULL, 0xce7a3ef96e15d3beULL},
            {4, 0x1dc8976100a8d6e9ULL, 0xb18dd1e836cce748ULL},
            {5, 0xceb756b4b862a402ULL, 0xf2c54d56ab37a6e4ULL},
            {7, 0xb04ed9e1dce3caeeULL, 0xedb210aa7a53d1a3ULL},
            {8, 0x1db4001ff93250ffULL, 0x1fe33ad9b1b99dddULL},
            {9, 0xbbfbd8a057866e9aULL, 0x41b10fc54558a1f0ULL},
            {12, 0x3628b951719e82d1ULL, 0x0db3f93af6aae3a6ULL},
            {16, 0xb4ba7fac410cad70ULL, 0xf5dc00dc04fc8449ULL},
            {32, 0x9df7ac85a74ce4d5ULL, 0x41d8a21760cffc70ULL},
            {64, 0xce2e7fe3f9702851ULL, 0x96a9ecc3d3e80fecULL},
            {128, 0x5f3055180725ea0dULL, 0x93c7f7e4e696ea66ULL}
        };
        Array<char> bytes(129);
        for (std::size_t i = 0; i < 128; ++i)
            bytes[i + 1] = static_cast<char>((i * 73 + 131) & 255);
        bool all_match = true;
        for (const auto& fixture : FIXTURES)
            all_match = all_match
                && bytes_hash(bytes.data() + 1, fixture.length) == fixture.standard_seed
                && bytes_hash(bytes.data() + 1, fixture.length, 1234) == fixture.custom_seed;
        UTEST_EXPECT(all_match);
        UTEST_EXPECT_EQ(bytes_hash(nullptr, 0), FIXTURES[0].standard_seed);
        const char unaligned[] = {0, 1, 2, 3, 4};
        UTEST_EXPECT_EQ(load_u32_le(unaligned + 1), 0x04030201u);
        const std::uint8_t unsigned_bytes[] = {0x01, 0x80, 0xff, 0x7f};
        UTEST_EXPECT_EQ(load_u32_le(unsigned_bytes), 0x7fff8001u);
        const std::byte raw_bytes[] = {std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};
        const auto* cursor = raw_bytes;
        static_assert(noexcept(load_u32_le(cursor)));
        UTEST_EXPECT_EQ(load_u32_le(cursor), 0x04030201u);
        UTEST_EXPECT(cursor == raw_bytes);
        const char* text = "stable";
        UTEST_EXPECT_EQ(ayrhash(text), bytes_hash(text, 6));
        UTEST_EXPECT_EQ(ayrhash("stable"), bytes_hash(text, 6));
    };

    return UTEST_COMPLETE();
}
