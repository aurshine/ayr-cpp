#include <chrono>
#include <memory>
#include <thread>
#include <tuple>

#include <ayr/base.hpp>
#include <ayr/base/utest.hpp>

using namespace ayr;

namespace
{
	struct Lifetime
	{
		static inline int alive = 0;
		int value = 0;

		Lifetime(int value = 0) : value(value) { ++alive; }
		Lifetime(const Lifetime& other) : value(other.value) { ++alive; }
		Lifetime(Lifetime&& other) noexcept : value(other.value) { ++alive; other.value = -1; }
		~Lifetime() { --alive; }

		Lifetime& operator=(const Lifetime&) = default;
		Lifetime& operator=(Lifetime&&) = default;
		auto operator<=>(const Lifetime&) const = default;
	};

	struct ReprValue
	{
		void __repr__(Buffer& buffer) const { buffer << "repr-value"; }
	};
}

int main()
{
	UTEST_SCOPE("测试 Array 的初始化、负下标、迭代和序列查询。")
	{
		Array<int> zeros(3);
		UTEST_EXPECT_EQ(zeros.size(), 3);
		UTEST_EXPECT_EQ(zeros[0], 0);
		UTEST_EXPECT_EQ(zeros[-1], 0);

		Array<int> values(4, 7);
		values[1] = 2;
		values[2] = 3;
		values[3] = 4;
		UTEST_EXPECT_EQ(values.front(), 7);
		UTEST_EXPECT_EQ(values.back(), 4);
		UTEST_EXPECT_EQ(values.index(3), 2);
		UTEST_EXPECT_EQ(values.index_if([](int value) { return value > 3; }, 1), 3);
		UTEST_EXPECT_EQ(values.index(99), -1);
		UTEST_EXPECT(values.contains(2));
		UTEST_EXPECT(!values.contains(99));

		int sum = 0;
		values.each([&](int value) { sum += value; });
		UTEST_EXPECT_EQ(sum, 16);
		values.each([](int& value) { value *= 2; });
		UTEST_EXPECT_EQ(values, Array<int>({ 14, 4, 6, 8 }));
	};

	UTEST_SCOPE("测试 Array 的工厂函数、比较、拷贝、移动和 resize。")
	{
		auto values = arr(1, 2L, 3);
		static_assert(std::is_same_v<typename decltype(values)::Value_t, long>);
		UTEST_EXPECT_EQ(values, Array<long>({ 1, 2, 3 }));
		UTEST_EXPECT(values < Array<long>({ 1, 2, 4 }));
		UTEST_EXPECT(values < Array<long>({ 1, 2, 3, 0 }));

		Array<long> copied(values);
		copied = copied;
		UTEST_EXPECT_EQ(copied, values);
		Array<long> moved(std::move(copied));
		UTEST_EXPECT_EQ(moved.size(), 3);
		UTEST_EXPECT_EQ(copied.size(), 0);
		Array<long> assigned(1);
		assigned = std::move(moved);
		assigned = std::move(assigned);
		UTEST_EXPECT_EQ(assigned, values);
		UTEST_EXPECT_EQ(moved.size(), 0);

		assigned.resize(2);
		UTEST_EXPECT_EQ(assigned.size(), 2);
		UTEST_EXPECT_EQ(assigned[0], 0);
	};

	UTEST_SCOPE("测试 Array::separate 转移所有权且析构次数正确。")
	{
		Lifetime::alive = 0;
		Array<Lifetime> values(2, Lifetime(9));
		UTEST_EXPECT_EQ(Lifetime::alive, 2);
		auto [data, size] = values.separate();
		UTEST_EXPECT_EQ(values.size(), 0);
		UTEST_EXPECT_EQ(size, 2);
		UTEST_EXPECT_EQ(data[1].value, 9);
		ayr_desloc(data, static_cast<size_t>(size));
		UTEST_EXPECT_EQ(Lifetime::alive, 0);
	};

	UTEST_SCOPE("测试 View 和 ViewOF 引用原对象、重绑定及函数调用。")
	{
		int first = 4;
		int second = 8;
		View view(first);
		UTEST_EXPECT_EQ(view.get<int>(), 4);
		view.get<int>() = 5;
		UTEST_EXPECT_EQ(first, 5);
		View copied(view);
		UTEST_EXPECT(copied == first);
		copied = second;
		UTEST_EXPECT_EQ(copied.get<int>(), 8);

		const auto first_view = view_of(first);
		UTEST_EXPECT_EQ(first_view.get(), 5);
		UTEST_EXPECT(first_view == first);
		auto function = [](int value) { return value * 3; };
		auto function_view = view_of(function);
		UTEST_EXPECT_EQ(function_view(7), 21);
	};

	UTEST_SCOPE("测试 Buffer 写入、查找、读取游标、原地整理和扩容。")
	{
		Buffer buffer(8);
		buffer.append_bytes("abcdef", 6);
		UTEST_EXPECT_EQ(buffer.readable_size(), 6);
		UTEST_EXPECT_EQ(buffer.find('c'), 2);
		UTEST_EXPECT_EQ(buffer.find("de"), 3);
		UTEST_EXPECT_EQ(buffer.find('a', -4), 0);
		UTEST_EXPECT_EQ(buffer.find("missing"), -1);
		buffer.retrieve(4);
		const char* old_data = buffer.data();
		buffer.append_bytes("WXYZ", 4);
		UTEST_EXPECT(buffer.data() == old_data);
		UTEST_EXPECT_EQ(vstr(buffer.peek(), buffer.readable_size()), "efWXYZ");

		buffer.append_bytes("0123456789", 10);
		UTEST_EXPECT(buffer.capacity() >= 16);
		UTEST_EXPECT_EQ(buffer.find("XYZ0"), 3);
		buffer.retrieve(999);
		UTEST_EXPECT_EQ(buffer.readable_size(), 0);
		buffer.written(-1);
		UTEST_EXPECT_EQ(buffer.readable_size(), 0);
	};

	UTEST_SCOPE("测试 Buffer 的复制、移动、格式化输出和关闭语义。")
	{
		Buffer source;
		source << true << ' ' << false << ' ' << 42 << ' ' << 1.5 << ' '
			<< std::pair<int, int>{ 1, 2 } << ' ' << std::tuple<int, const char*>{ 3, "x" }
			<< ' ' << nullptr << ' ' << ReprValue{};
		CString text = vstr(source.peek(), source.readable_size());
		UTEST_EXPECT(text.contains("true false 42"));
		UTEST_EXPECT(text.contains("(1, 2)"));
		UTEST_EXPECT(text.contains("(3, x)"));
		UTEST_EXPECT(text.endswith("nullptr repr-value"));

		Buffer copied(source);
		copied = copied;
		UTEST_EXPECT_EQ(vstr(copied.peek(), copied.readable_size()), text);
		Buffer moved(std::move(copied));
		UTEST_EXPECT_EQ(vstr(moved.peek(), moved.readable_size()), text);
		Buffer assigned;
		assigned = std::move(moved);
		assigned = std::move(assigned);
		UTEST_EXPECT_EQ(vstr(assigned.peek(), assigned.readable_size()), text);
		assigned.close();
		UTEST_EXPECT(assigned.closed());
		UTEST_EXPECT_EQ(assigned.capacity(), -1);
		UTEST_EXPECT_EQ(assigned.writeable_size(), -1);
		const c_size before = assigned.readable_size();
		assigned.append_bytes("ignored", 7);
		UTEST_EXPECT_EQ(assigned.readable_size(), before);
	};

	UTEST_SCOPE("测试位运算、哈希与内存辅助函数。")
	{
		UTEST_EXPECT_EQ(lowbit(12), 4);
		UTEST_EXPECT_EQ(lowbit_index(8), 3);
		UTEST_EXPECT_EQ(highbit_index(9), 3);
		UTEST_EXPECT_EQ(highbit(9), 8);
		UTEST_EXPECT(all_one(7));
		UTEST_EXPECT(!all_one(6));
		UTEST_EXPECT(only_one(8));
		UTEST_EXPECT(!only_one(10));
		UTEST_EXPECT_EQ(roundup2(0), 1);
		UTEST_EXPECT_EQ(roundup2(17), 32);
		UTEST_EXPECT_EQ(decode_fixed32("\x01\x02\x03\x04"), 0x04030201u);
		UTEST_EXPECT_EQ(ayrhash("stable"), ayrhash("stable"));
		UTEST_EXPECT(ayrhash("stable") != ayrhash("different"));

		auto value = std::unique_ptr<Lifetime, AyrDeleter<Lifetime>>(ayr_make<Lifetime>(12));
		UTEST_EXPECT_EQ(value->value, 12);
	};

	UTEST_SCOPE("测试 AyrError 的名称、消息、详情和复制移动。")
	{
		AyrError error("SampleError", "bad value");
		UTEST_EXPECT(error.is("SampleError"));
		UTEST_EXPECT_EQ(error.name(), "SampleError");
		UTEST_EXPECT_EQ(error.error(), "SampleError: bad value");
		UTEST_EXPECT(error.details().contains("bad value"));
		UTEST_EXPECT(vstr(error.what()).contains("SampleError"));
		AyrError copied(error);
		AyrError moved(std::move(copied));
		UTEST_EXPECT_EQ(moved.error(), "SampleError: bad value");
		UTEST_EXPECT_AYR_ERROR(error.raise());
	};

	UTEST_SCOPE("测试 Date、Timer 和作用域退出任务。")
	{
		Date epoch(0);
		UTEST_EXPECT_EQ(epoch.year(), 1970);
		UTEST_EXPECT_EQ(epoch.month(), 1);
		UTEST_EXPECT_EQ(epoch.day(), 1);
		UTEST_EXPECT_EQ(epoch.week_str(), "Thu");
		UTEST_EXPECT(Date::check_run_year(2000));
		UTEST_EXPECT(!Date::check_run_year(1900));
		UTEST_EXPECT(Date(2024, 2, 29, 0, 0, 0) < Date(2024, 3, 1, 0, 0, 0));

		Timer_ms timer;
		UTEST_EXPECT_AYR_ERROR(timer.escape());
		timer.into();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		UTEST_EXPECT(timer.escape() >= 0.0);
		UTEST_EXPECT(timer([] {}) >= 0.0);

		int exits = 0;
		{
			exitask([&] { ++exits; });
			UTEST_EXPECT_EQ(exits, 0);
		}
		UTEST_EXPECT_EQ(exits, 1);
	};

	return UTEST_COMPLETE();
}
