#include <atomic>

#include <ayr/async.hpp>
#include <ayr/base.hpp>

#include <ayr/base/utest.hpp>

using namespace ayr;

int main()
{
	// 测试 exitask 在离开作用域时执行清理逻辑。
	std::atomic<bool> cleaned = false;
	{
		exitask([&] { cleaned = true; });
		AYR_TEST_EXPECT(!cleaned.load());
	}
	AYR_TEST_EXPECT(cleaned.load());

	// 测试多个 exitask 按栈对象析构顺序执行。
	CString order;
	{
		exitask([&] { order += "B"; });
		exitask([&] { order += "A"; });
	}
	AYR_TEST_EXPECT_EQ(order, "AB");
}
