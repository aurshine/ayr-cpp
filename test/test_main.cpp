#include <ayr/base/Atring.hpp>
#include <ayr/base/utest.hpp>

using namespace ayr;

int main()
{
	UTEST_SCOPE("测试空测试块能够正常运行。")
	{
	};

	UTEST_SCOPE("测试第二个测试块能够继续运行。")
	{
		throw AyrError("Error", "112");
	};

	return UTEST_COMPLETE();
}
