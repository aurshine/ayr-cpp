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
	class TestRecorder
	{
		using self = TestRecorder;

		c_size total_count_, failed_count_;

		ColorPrinter info_print, success_print, failed_print;

		TestRecorder() : 
			total_count_(0), 
			failed_count_(0),
			info_print(stdout, Color::WHITE),
			success_print(stdout, Color::GREEN),
			failed_print(stdout, Color::RED)
		{ 
			AyrError::show(false); 
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
		void fail(const CString& action, const CString& expr, const CString& file, int line)
		{
			++failed_count_;
			this->action(failed_print, vstr(ayr::format("[FALIED  {}]", action)), expr, file, line);
		}

		void fail(const CString& action, const CString& expr, const CString& file, int line, CString ex_msg)
		{
			++failed_count_;
			this->action(failed_print, vstr(ayr::format("[FALIED  {}]", action)), expr, file, line, ex_msg);
		}

		void success(const CString& action, const CString& expr, const CString& file, int line)
		{
			this->action(success_print, vstr(ayr::format("[SUCCESS {}]", action)), expr, file, line);
		}

		void action(ColorPrinter& cp, const CString& action, const CString& expr, const CString& file, int line)
		{
			Buffer buffer;
			buffer << action << " " << file << ":" << line << " [" << expr << "]\n";
			cp.write_from_buffer(buffer);
		}

		void action(ColorPrinter& cp, const CString& action, const CString& expr, const CString& file, int line, CString ex_msg)
		{
			Buffer buffer;
			buffer << action << " " << file << ":" << line << " [" << expr << "] -> [" << ex_msg << "]\n";
			cp.write_from_buffer(buffer);
		}
	public:
		TestRecorder(const self&) = delete;

		self& operator=(const self&) = delete;

		/*
		* @brief 获取当前测试可执行文件内唯一的测试运行器。
		*
		* @return TestRecorder& 测试运行器单例
		*/
		static self& instance()
		{
			static self unit_test;
			return unit_test;
		}

		template<typename... Args>
		void print(Args... args) const { info_print(args...); }

		void print_from_buffer(const Buffer& buffer) { info_print.write_from_buffer(buffer); }

		// 完成utest，并返回错误的数量
		int complete() const
		{
			Buffer buffer;
			if (failed_count_)
			{
				buffer << "\nUnit Test Failed: " << failed_count_ << " / " << total_count_ << "\n";
				failed_print.write_from_buffer(buffer);
			}
			else
			{
				buffer << "\nUnit Test Passed: " << total_count_ << "\n";
				success_print.write_from_buffer(buffer);
			}

			return failed_count_;
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
				fail("CONDITION", expr, file, line);
			else
				success("CONDITION", expr, file, line);
		}

		template<typename T1, typename T2>
		void expect_eq(const T1& t1, const T2& t2, const CString& expr, const CString& file, int line)
		{
			++total_count_;
			if (t1 != t2)
				fail("EQUAL    ", expr, file, line, vstr(ayr::format("{} != {}", cstr(t1), cstr(t2))));
			else
				success("EQUAL    ", expr, file, line);
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
			double diff = ifelse(actual > expected, actual - expected, expected - actual);
			if (diff > eps)
				fail("NEAREQUAL", expr, file, line, vstr(ayr::format("|{} - {}| > {}", actual, expected, diff)));
			else
				success("NEAREQUAL", expr, file, line);
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
				success("RAISEERR ", expr, file, line);
				return;
			}
			fail("RAISEERR ", expr, file, line);
		}

		template<typename F>
		void expect_run_expr(F&& fn, const CString& expr, const CString& file, int line)
		{
			++total_count_;
			try
			{
				fn();
			}
			catch (const AyrError& e)
			{
				fail("RUNEXPR  ", expr, file, line, e.error());
				return;
			}
			success("RUNEXPR  ", expr, file, line);
		}

		template<typename F>
		void expect_run_scope(F&& fn, const CString& expr, const CString& file, int line)
		{
			++total_count_;
			try
			{
				fn();
			}
			catch (const AyrError& e)
			{
				fail("RUNSCOPE ", expr, file, line, e.error());
				return;
			}
			success("RUNSCOPE ", expr, file, line);
		}
	};
	
	class TestScoper
	{
		CString scope_name_, file_;

		int line_;
	public:
		TestScoper(CString scope_name, const char* file, int line)
			: scope_name_(scope_name), file_(file), line_(line) {}

		template<typename F>
		void operator+(F&& scope) const
		{
			TestRecorder::instance().print("[START_TEST_SCOPE ]", scope_name_);
			TestRecorder::instance().expect_run_scope(scope, scope_name_, file_, line_);
		}
	};

#define UTEST_SCOPE(scope_name) TestScoper{scope_name, __FILE__, __LINE__} + [&]()

#define UTEST_EXPECT(expr) TestRecorder::instance().expect(static_cast<bool>(expr), #expr, __FILE__, __LINE__)

#define UTEST_EXPECT_EQ(actual, expected) TestRecorder::instance().expect_eq((actual), (expected), #actual " == " #expected, __FILE__, __LINE__)

#define UTEST_EXPECT_NEAR(actual, expected, eps) TestRecorder::instance().expect_near((actual), (expected), (eps), #actual " ~= " #expected, __FILE__, __LINE__)

#define UTEST_EXPECT_AYR_ERROR(expr) TestRecorder::instance().expect_ayr_error([&] { expr; }, #expr, __FILE__, __LINE__)

#define UTEST_EXPR(expr) TestRecorder::instance().expect_run_expr([&] { expr; }, #expr, __FILE__, __LINE__)

#define UTEST_COMPLETE() TestRecorder::instance().complete()
}

#endif // AYR_BASE_UTEST_HPP
