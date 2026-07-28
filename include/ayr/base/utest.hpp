#ifndef AYR_BASE_UTEST_HPP
#define AYR_BASE_UTEST_HPP

#include "raise_error.hpp"

namespace ayr
{
	
	/*
	* @brief 轻量单元测试运行器。
	*
	* @details
	* 每个测试可执行文件内使用一个单例对象记录断言数量和失败数量。
	* 析构时自动输出汇总结果，测试文件无需手动调用 result。
	*/
	class UnitTest
	{
		using self = UnitTest;

		c_size total_count_, failed_count_;

		UnitTest() : total_count_(0), failed_count_(0) {}
	public:
		~UnitTest()
		{
			ColorPrinter cp(stdout, Color::AQUA);
			if (failed_count_)
				cp("\nUnit Test Failed: ", failed_count_, "/", total_count_);
			else
				cp("\nUnit Test Passed: ", total_count_);
		}

		UnitTest(const self&) = delete;

		self& operator=(const self&) = delete;

		/*
		* @brief 获取当前测试可执行文件内唯一的测试运行器。
		*
		* @return UnitTest& 测试运行器单例
		*/
		static self& instance()
		{
			static self unit_test;
			return unit_test;
		}

		/*
		* @brief 记录一次单元测试失败，并输出可定位的错误信息。
		*
		* @details
		* 这里不抛出 RuntimeError。RuntimeError 表示不可恢复错误，会中断程序；
		* 单元测试断言失败应继续执行后续检查，方便一次运行暴露更多问题。
		*
		* @param expr 失败的表达式文本
		* @param file 失败所在文件
		* @param line 失败所在行号
		*/
		void fail(const CString& expr, const CString& file, int line)
		{
			++failed_count_;
			ayr_warner("[CHECK FALIED] ", file, ":", line, ":", expr);
		}

		void success(const CString& expr, const CString& file, int line)
		{
			print("[CHECK SUCCESS]", file, ":", line, ":", expr);
		}

		/*
		* @brief 检查布尔条件是否成立。
		*
		* @param condition 被检查的布尔值
		* @param expr 被检查的表达式文本
		* @param file 检查所在文件
		* @param line 检查所在行号
		*/
		void expect(bool condition, const CString& expr, const CString& file, int line)
		{
			++total_count_;
			if (!condition)
				fail(expr, file, line);
			else
				success(expr, file, line);
		}

		/*
		* @brief 检查两个浮点数是否在允许误差内近似相等。
		*
		* @param actual 实际值
		* @param expected 期望值
		* @param eps 可接受误差
		* @param expr 被检查的表达式文本
		* @param file 检查所在文件
		* @param line 检查所在行号
		*/
		void expect_near(double actual, double expected, double eps, const CString& expr, const CString& file, int line)
		{
			++total_count_;
			double diff = actual > expected ? actual - expected : expected - actual;
			if (diff > eps)
				fail(expr, file, line);
			else
				success(expr, file, line);
		}

		/*
		* @brief 检查表达式是否抛出 AyrError。
		*
		* @details
		* 只捕获 AyrError，避免把其他未知异常误认为预期行为。
		*
		* @param fn 待执行的测试函数
		* @param expr 被检查的表达式文本
		* @param file 检查所在文件
		* @param line 检查所在行号
		*/
		template<typename F>
		void expect_ayr_error(F&& fn, const CString& expr, const CString& file, int line)
		{
			++total_count_;
			try
			{
				fn();
			}
			catch (const AyrError&)
			{
				success(expr, file, line);
				return;
			}
			fail(expr, file, line);
		}
	};

#define AYR_TEST_EXPECT(expr) UnitTest::instance().expect(static_cast<bool>(expr), #expr, __FILE__, __LINE__)

#define AYR_TEST_EXPECT_EQ(actual, expected) AYR_TEST_EXPECT((actual) == (expected))

#define AYR_TEST_EXPECT_NEAR(actual, expected, eps) UnitTest::instance().expect_near((actual), (expected), (eps), #actual " ~= " #expected, __FILE__, __LINE__)

#define AYR_TEST_EXPECT_AYR_ERROR(expr) UnitTest::instance().expect_ayr_error([&] { expr; }, #expr, __FILE__, __LINE__)
}


#endif // AYR_BASE_UTEST_HPP
