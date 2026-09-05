#include <ayr/air/Optional.hpp>
#include <ayr/air/Table.hpp>
#include <ayr/base/utest.hpp>

using namespace ayr;

namespace
{
	struct Tracked
	{
		static inline int alive = 0;
		int value;

		Tracked(int value) : value(value) { ++alive; }
		Tracked(const Tracked& other) : value(other.value) { ++alive; }
		Tracked(Tracked&& other) noexcept : value(other.value) { ++alive; other.value = -1; }
		~Tracked() { --alive; }

		Tracked& operator=(const Tracked&) = default;
		Tracked& operator=(Tracked&&) = default;
		auto operator<=>(const Tracked&) const = default;
	};
}

int main()
{
	UTEST_SCOPE("测试 Optional 的空值访问、value_or、赋值和 reset。")
	{
		Optional<CString> empty;
		UTEST_EXPECT(!empty);
		UTEST_EXPECT(!empty.has_value());
		UTEST_EXPECT_AYR_ERROR(empty.value());
		CString fallback = dstr("fallback");
		UTEST_EXPECT_EQ(empty.value_or(fallback), "fallback");

		empty = dstr("value");
		UTEST_EXPECT(empty);
		UTEST_EXPECT_EQ(*empty, "value");
		UTEST_EXPECT_EQ(empty->size(), 5);
		UTEST_EXPECT_EQ(empty.value_or(fallback), "value");
		empty.emplace(dstr("xxx"));
		UTEST_EXPECT_EQ(*empty, "xxx");
		empty.reset();
		empty.reset();
		UTEST_EXPECT(!empty);
	};

	UTEST_SCOPE("测试 Optional 的拷贝、移动、自赋值和生命周期。")
	{
		Tracked::alive = 0;
		Optional<Tracked> first(Tracked(7));
		UTEST_EXPECT_EQ(Tracked::alive, 1);
		Optional<Tracked> copied(first);
		UTEST_EXPECT_EQ(Tracked::alive, 2);
		copied = copied;
		UTEST_EXPECT_EQ(copied->value, 7);
		Optional<Tracked> moved(std::move(copied));
		UTEST_EXPECT(!copied);
		UTEST_EXPECT_EQ(moved->value, 7);
		first = std::move(moved);
		first = std::move(first);
		UTEST_EXPECT(!moved);
		UTEST_EXPECT_EQ(first->value, 7);
	};
	UTEST_EXPECT_EQ(Tracked::alive, 0);

	UTEST_SCOPE("测试 Optional 的 map、or_else、filter、比较、哈希和文本表示。")
	{
		Optional<int> value(5);
		Optional<int> empty;
		auto doubled = value.map([](int number) { return number * 2; });
		auto absent = empty.map([](int number) { return number * 2; });
		auto chained = value.and_then([](int number) { return Optional<CString>(cstr(number)); });
		auto empty_chain = empty.and_then([](int number) { return Optional<CString>(cstr(number)); });
		auto transformed = value.transform([](int number) { return Optional<int>(number + 1); });
		UTEST_EXPECT_EQ(*doubled, 10);
		UTEST_EXPECT(!absent);
		UTEST_EXPECT_EQ(*chained, "5");
		UTEST_EXPECT(!empty_chain);
		UTEST_EXPECT_EQ(*transformed, 6);
		UTEST_EXPECT_EQ(*empty.or_else([] { return Optional<int>(9); }), 9);
		UTEST_EXPECT_EQ(*value.or_else([] { return Optional<int>(9); }), 5);
		UTEST_EXPECT(value.filter([](int number) { return number % 2 == 1; }));
		UTEST_EXPECT(!value.filter([](int number) { return number > 10; }));
		UTEST_EXPECT(value > empty);
		UTEST_EXPECT_EQ(value, Optional<int>(5));
		UTEST_EXPECT_EQ(ayrhash(value), ayrhash(5));
		UTEST_EXPECT_EQ(ayrhash(empty), 0);
		UTEST_EXPECT_EQ(cstr(value), "5");
		UTEST_EXPECT_EQ(cstr(empty), "None");
	};

	UTEST_SCOPE("测试 Pow2Policy 的阈值、索引、扩缩容和容量适配。")
	{
		Pow2Policy policy;
		UTEST_EXPECT_EQ(policy.capacity(), 16);
		UTEST_EXPECT_EQ(policy.load_threshold(), 12);
		UTEST_EXPECT_EQ(policy.mask(), 15);
		UTEST_EXPECT_EQ(policy.hash2index(31), 15);
		UTEST_EXPECT_EQ(policy.next_index(15), 0);
		UTEST_EXPECT_EQ(policy.expand_capacity(), 32);
		UTEST_EXPECT_EQ(policy.shrink_capacity(), 16);
		UTEST_EXPECT_EQ(policy.adapt_at_least(12), 32);
		UTEST_EXPECT_EQ(policy.adapt_at_least(1), 16);
		UTEST_EXPECT_EQ(policy.reset(), 16);
		Pow2Policy copied(policy);
		copied = copied;
		UTEST_EXPECT_EQ(copied.capacity(), 16);
	};

	UTEST_SCOPE("测试 RobinItem 的空闲、写入、替换、交换、复制和移动。")
	{
		RobinItem<CString> item;
		UTEST_EXPECT(!item.used());
		item.set_empty_value(11, 2, "one");
		UTEST_EXPECT(item.used());
		UTEST_EXPECT_EQ(item.value(), "one");
		item.set_new_value("two");
		UTEST_EXPECT_EQ(item.value(), "two");
		hash_t hash = 22;
		RobinItem<CString>::Dist_t dist = 4;
		CString value = dstr("other");
		item.swap_elements(hash, dist, value);
		UTEST_EXPECT_EQ(item.hashv, 22);
		UTEST_EXPECT_EQ(item.dist, 4);
		UTEST_EXPECT_EQ(item.value(), "other");
		UTEST_EXPECT_EQ(hash, 11);
		UTEST_EXPECT_EQ(value, "two");

		RobinItem<CString> copied(item);
		RobinItem<CString> moved(std::move(copied));
		UTEST_EXPECT(!copied.used());
		UTEST_EXPECT_EQ(moved.value(), "other");
		moved = moved;
		moved.set_unused();
		UTEST_EXPECT(!moved.used());
	};

	UTEST_SCOPE("测试 Table 的碰撞处理、覆盖、删除回移和扩容。")
	{
		Table<CString> table;
		UTEST_EXPECT(table.empty());
		for (int i = 0; i < 12; ++i)
			table.insert(static_cast<hash_t>(i * 16), cstr(i));
		UTEST_EXPECT_EQ(table.size(), 12);
		UTEST_EXPECT_EQ(table.capacity(), 32);
		for (int i = 0; i < 12; ++i)
			UTEST_EXPECT(table.contains(static_cast<hash_t>(i * 16)));

		table.insert(16, "updated");
		UTEST_EXPECT_EQ(table.size(), 12);
		auto [index, distance] = table.try_get(16);
		UTEST_EXPECT(table.items_[index].used());
		UTEST_EXPECT_EQ(table.items_[index].value(), "updated");
		UTEST_EXPECT(distance >= 0);
		UTEST_EXPECT(table.pop(0));
		UTEST_EXPECT(!table.contains(0));
		UTEST_EXPECT(table.contains(16));
		UTEST_EXPECT(!table.pop(999));
	};

	UTEST_SCOPE("测试 Table 的拷贝、移动、赋值、repr 和 clear。")
	{
		Table<int> original;
		original.insert(1, 10);
		original.insert(17, 20);
		Table<int> copied(original);
		copied = copied;
		UTEST_EXPECT(copied.contains(1));
		UTEST_EXPECT(copied.contains(17));
		Table<int> moved(std::move(copied));
		UTEST_EXPECT_EQ(copied.size(), 0);
		UTEST_EXPECT_EQ(moved.size(), 2);
		Table<int> assigned;
		assigned = std::move(moved);
		assigned = std::move(assigned);
		UTEST_EXPECT(assigned.contains(17));
		UTEST_EXPECT(cstr(assigned).contains("value:10"));
		assigned.clear();
		UTEST_EXPECT(assigned.empty());
		UTEST_EXPECT_EQ(assigned.capacity(), 16);
	};

	UTEST_SCOPE("测试扩容后的 Table::clear 析构所有已存对象。")
	{
		Tracked::alive = 0;
		Table<Tracked> tracked;
		for (int i = 0; i < 12; ++i)
			tracked.insert(static_cast<hash_t>(i * 16), i);
		UTEST_EXPECT_EQ(Tracked::alive, 12);
		UTEST_EXPECT_EQ(tracked.capacity(), 32);
		tracked.clear();
		UTEST_EXPECT_EQ(Tracked::alive, 0);
		UTEST_EXPECT(tracked.empty());
	};

	return UTEST_COMPLETE();
}
