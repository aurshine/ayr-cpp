#include <limits>

#include <ayr/air/Dict.hpp>
#include <ayr/air/Set.hpp>
#include <ayr/air/Table.hpp>
#include <ayr/base/utest.hpp>

using namespace ayr;

namespace
{
    /** @brief 可指定哈希并统计相等比较次数的键，用于验证碰撞和重复比较。 */
    struct CountingKey
    {
        static inline int comparisons = 0;
        int value = 0;
        hash_t hash = 0;

        hash_t __hash__() const { return hash; }

        bool operator==(const CountingKey& other) const
        {
            ++comparisons;
            return value == other.value;
        }
    };

    /** @brief 直接持有测试键值，使 Table 测试无需管理链表节点。 */
    struct TestItem
    {
        using Key = CountingKey;
        using Val = int;

        Key key_;
        Val value_ = 0;

        const Key& key() const { return key_; }
        Val& value() { return value_; }
        const Val& value() const { return value_; }
        bool key_equal(const Key& key) const { return key_ == key; }
        hash_t hashv() const { return key_.__hash__(); }
    };

    using TestTable = Table<TestItem>;

    /** @brief 使用探测结果提供的索引和距离插入新测试表项。 */
    void insert_item(TestTable& table, CountingKey key, int value)
    {
        const auto result = table.probe(key, key.__hash__());
        if (!result.found)
            table.insert_value_on_index(result.index, result.dist, TestItem{ key, value });
    }
}

int main()
{
    UTEST_SCOPE("容量策略保持最小容量及严格负载阈值，支持完整的 64 位容量边界。")
    {
        static_assert([] { Pow2Policy policy; return policy.adapt_at_least(12) == 32; }());
        Pow2Policy policy;
        UTEST_EXPECT_EQ(policy.adapt_at_least(0), 16);
        UTEST_EXPECT_EQ(policy.adapt_at_least(11), 16);
        UTEST_EXPECT_EQ(policy.adapt_at_least(12), 32);
        bool all_valid = true;
        for (int bit = 4; bit <= 62; ++bit)
        {
            const c_size capacity = c_size{1} << bit;
            const c_size threshold = capacity - (capacity >> 2);
            all_valid = all_valid && policy.adapt_at_least(threshold - 1) == capacity;
            if (bit < 62)
                all_valid = all_valid && policy.adapt_at_least(threshold) == capacity * 2;
        }
        UTEST_EXPECT(all_valid);
        const Pow2Policy& const_policy = policy;
        UTEST_EXPECT_EQ(const_policy.load_threshold(), policy.capacity() - (policy.capacity() >> 2));
        policy.adapt_at_least(100);
        const c_size capacity = policy.capacity();
        const c_size limit = policy.max_capacity() - (policy.max_capacity() >> 2);
        UTEST_EXPECT_AYR_ERROR(policy.adapt_at_least(-1));
        UTEST_EXPECT_EQ(policy.capacity(), capacity);
        UTEST_EXPECT_AYR_ERROR(policy.adapt_at_least(limit));
        UTEST_EXPECT_AYR_ERROR(policy.adapt_at_least(std::numeric_limits<c_size>::max()));
        UTEST_EXPECT_EQ(policy.capacity(), capacity);
    };

    UTEST_SCOPE("空表探测返回插入位置，不访问空槽位中的键。")
    {
        TestTable table;
        CountingKey::comparisons = 0;
        const TestTable::ProbeResult result = table.probe(CountingKey{ 1, 31 }, 31);
        UTEST_EXPECT(!result.found);
        UTEST_EXPECT_EQ(result.index, 15);
        UTEST_EXPECT_EQ(result.dist, 0);
        UTEST_EXPECT_EQ(CountingKey::comparisons, 0);
        UTEST_EXPECT(table.empty());
    };

    UTEST_SCOPE("遇到距离更小的占用槽位时提前结束，并提供正确的 Robin Hood 插入起点。")
    {
        TestTable table;
        insert_item(table, CountingKey{ 0, 0 }, 10);
        insert_item(table, CountingKey{ 1, 1 }, 20);
        CountingKey::comparisons = 0;
        const auto missing = table.probe(CountingKey{ 16, 16 }, 16);
        UTEST_EXPECT(!missing.found);
        UTEST_EXPECT_EQ(missing.index, 1);
        UTEST_EXPECT_EQ(missing.dist, 1);
        UTEST_EXPECT_EQ(CountingKey::comparisons, 0);
        table.insert_value_on_index(missing.index, missing.dist, TestItem{ CountingKey{ 16, 16 }, 30 });
        const auto displaced = table.probe(CountingKey{ 1, 1 }, 1);
        UTEST_EXPECT(displaced.found);
        UTEST_EXPECT_EQ(displaced.index, 2);
        UTEST_EXPECT_EQ(displaced.dist, 1);
        UTEST_EXPECT_EQ(table.items_[displaced.index].value(), 20);
    };

    UTEST_SCOPE("完整哈希碰撞及表尾回绕时，命中和缺失结果正确，删除后仍能找到剩余键。")
    {
        TestTable table;
        for (int key : { 10, 20, 30 })
            insert_item(table, CountingKey{ key, 15 }, key);
        CountingKey::comparisons = 0;
        const auto found = table.probe(CountingKey{ 20, 15 }, 15);
        UTEST_EXPECT(found.found);
        UTEST_EXPECT_EQ(found.index, 0);
        UTEST_EXPECT_EQ(found.dist, 1);
        UTEST_EXPECT_EQ(CountingKey::comparisons, 2);
        const auto missing = table.probe(CountingKey{ 40, 15 }, 15);
        UTEST_EXPECT(!missing.found);
        UTEST_EXPECT_EQ(missing.index, 2);
        UTEST_EXPECT_EQ(missing.dist, 3);
        UTEST_EXPECT_EQ(table.size(), 3);
        UTEST_EXPECT(table.pop_value_on_index(found.index));
        const auto moved = table.probe(CountingKey{ 30, 15 }, 15);
        UTEST_EXPECT(moved.found);
        UTEST_EXPECT_EQ(moved.index, 0);
        UTEST_EXPECT_EQ(moved.dist, 1);
        UTEST_EXPECT(!table.probe(CountingKey{ 20, 15 }, 15).found);
    };

    UTEST_SCOPE("多次扩容后，探测的索引、距离和值与重排后的槽位一致。")
    {
        TestTable table;
        for (int key = 0; key < 128; ++key)
            insert_item(table, CountingKey{ key, static_cast<hash_t>(key) * 16 }, key * 3);
        bool all_valid = true;
        const TestTable& const_table = table;
        for (int key = 0; key < 128; ++key)
        {
            const hash_t hash = static_cast<hash_t>(key) * 16;
            const auto result = const_table.probe(CountingKey{ key, hash }, hash);
            all_valid = all_valid && result.found
                && const_table.items_[result.index].value() == key * 3
                && result.dist == ((result.index - const_table.policy_.hash2index(hash)
                    + const_table.capacity()) & const_table.policy_.mask());
        }
        UTEST_EXPECT(all_valid);
        UTEST_EXPECT_EQ(table.size(), 128);
        UTEST_EXPECT(table.capacity() > 16);
    };

    UTEST_SCOPE("Dict 查询和更新复用探测命中状态，同一个命中槽位只比较一次键。")
    {
        const CountingKey key{ 42, 42 };
        Dict<CountingKey, int> values;
        values.insert(key, 7);
        int fallback = -1;
        CountingKey::comparisons = 0;
        UTEST_EXPECT(values.contains(key));
        UTEST_EXPECT_EQ(CountingKey::comparisons, 1);
        CountingKey::comparisons = 0;
        UTEST_EXPECT_EQ(values.get(key, fallback), 7);
        UTEST_EXPECT_EQ(CountingKey::comparisons, 1);
        const auto& const_values = values;
        CountingKey::comparisons = 0;
        UTEST_EXPECT_EQ(const_values.get(key, fallback), 7);
        UTEST_EXPECT_EQ(CountingKey::comparisons, 1);
        CountingKey::comparisons = 0;
        UTEST_EXPECT_EQ(values.insert(key, 8), 8);
        UTEST_EXPECT_EQ(CountingKey::comparisons, 1);
        CountingKey::comparisons = 0;
        UTEST_EXPECT_EQ(values.setdefault(key, 9), 8);
        UTEST_EXPECT_EQ(CountingKey::comparisons, 1);
        CountingKey::comparisons = 0;
        UTEST_EXPECT_EQ(values[key], 8);
        UTEST_EXPECT_EQ(CountingKey::comparisons, 1);
        CountingKey::comparisons = 0;
        values.pop(key);
        UTEST_EXPECT_EQ(CountingKey::comparisons, 1);
        UTEST_EXPECT(values.empty());
    };

    UTEST_SCOPE("Set 查询、重复插入和删除复用探测命中状态，只比较一次键。")
    {
        const CountingKey key{ 42, 42 };
        Set<CountingKey> values;
        values.insert(key);
        CountingKey::comparisons = 0;
        UTEST_EXPECT(values.contains(key));
        UTEST_EXPECT_EQ(CountingKey::comparisons, 1);
        CountingKey::comparisons = 0;
        UTEST_EXPECT_EQ(values.insert(key).value, 42);
        UTEST_EXPECT_EQ(CountingKey::comparisons, 1);
        UTEST_EXPECT_EQ(values.size(), 1);
        CountingKey::comparisons = 0;
        values.pop(key);
        UTEST_EXPECT_EQ(CountingKey::comparisons, 1);
        UTEST_EXPECT(values.empty());
    };

    return UTEST_COMPLETE();
}
