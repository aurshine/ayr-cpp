#include <atomic>

#include <ayr/async.hpp>
#include <ayr/base.hpp>
#include <ayr/net/Selector/IoResult.hpp>

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

	// IoResult失败时应同时保存错误码和可独立存活的详细错误信息。
	net::IoResult io_result;
#if defined(AYR_WIN)
	io_result.set_result(0, WSAECONNREFUSED);
	AYR_TEST_EXPECT(io_result.errstr.contains("Winsock error"));
#else
	io_result.set_result(0, ECONNREFUSED);
	AYR_TEST_EXPECT(io_result.errstr.contains("errno"));
#endif
	AYR_TEST_EXPECT(!io_result.ok());
	AYR_TEST_EXPECT(!io_result.errstr.empty());
	AYR_TEST_EXPECT(io_result.errstr.contains(": "));

	net::IoResult copied_result = io_result;
	io_result.set_result(5, 0);
	AYR_TEST_EXPECT(io_result.ok());
	AYR_TEST_EXPECT(io_result.errstr.empty());
	AYR_TEST_EXPECT(!copied_result.errstr.empty());
}
