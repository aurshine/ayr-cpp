#pragma once
#include <format>
#include <string>
#include <chrono>

#include <law/Wrapper.hpp>

namespace ayr
{
	// ����������
	struct YearMonthDay { int year, month, day; };

	// 1970_01_01�������ڵ�����
	class Date: public Object 
	{
	public:
		Date(int _day_from_1970_01_01_ = 0) : day_from_1970_01_01_(_day_from_1970_01_01_) {}

		Date(int year, int month, int day)
			: day_from_1970_01_01_(get_day_from_1970_01_01(year, month, day)) {}

		Date(const YearMonthDay& ymd) : Date(ymd.year, ymd.month, ymd.day) {}

		void swap(Date& date) { std::swap(day_from_1970_01_01_, date.day_from_1970_01_01_); }

		int year() const { return year_month_day().year; }

		int month() const { return year_month_day().month; }

		int day() const { return year_month_day().day; }

		int week_day() const { return (WEEK_DAY_FROM_1970_01_01 + day_from_1970_01_01_) % 7; }

		int day_from_1970_01_01() const { return day_from_1970_01_01_; }

		YearMonthDay year_month_day() const
		{
			int year = 1970, month = 1, day = 1;
			int temp = day_from_1970_01_01_;

			while (temp)
			{
				int is_run = (check_run_year(year) ? 1 : 0);
				int y = 365 + is_run;
				int m = MONTHS[month] + (month == 2 ? is_run : 0);

				if (temp - y >= 0)
				{
					year++;
					temp -= y;
				}
				else if (temp - m >= 0)
				{
					month++;
					temp -= m;
				}
				else
				{
					day += temp;
					temp = 0;
				}
			}

			return { year, month, day };
		}

		const char* __str__() const
		{
			YearMonthDay ymd = year_month_day();

			auto&& str = std::format("{:04d}-{:02d}-{:02d}", ymd.year, ymd.month, ymd.day);

			memcpy__str_buffer__(str.c_str(), str.size());
			return __str_buffer__;
		}

		cmp_t __cmp__(const Date& date) const
		{
			return day_from_1970_01_01_ - date.day_from_1970_01_01_;
		}

		static bool check_run_year(int year) { return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0); }

		static int get_day_from_1970_01_01(int year, int month, int day)
		{
			int cnt = day - 1;
			while (year != 1970)
			{
				cnt += 365 + (check_run_year(year) ? 1 : 0);
				year--;
			}

			while (month != 1)
			{
				cnt += MONTHS[month];
				month--;
			}

			return cnt;
		}

		static const int WEEK_DAY_FROM_1970_01_01 = 4;

		static int MONTHS[13];
	private:
		int day_from_1970_01_01_;
	};

	int Date::MONTHS[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };


	class Timer : public VoidWrapper
	{
	public:
		Timer() = default;

		Timer(const CString& sec_option)
		{
			if (sec_option == "s")
				dvd = 1000000;
			else if (sec_option == "ms")
				dvd = 1000;
			else if (sec_option == "us")
				dvd = 1;
			else
				error_assert(false, std::format("ValueError: invalid option for Timer(sec_option)"));
		}


		void befor_function() override
		{
			start_time = std::chrono::high_resolution_clock::now();
		}

		void after_function() override
		{
			CString sign = "us";
			if (dvd == 1000)
				sign = "ms";
			else if (dvd == 1000000)
				sign = "s";
			else if (dvd == 1)
				sign = "us";
			print(std::format("pass time: {}", get_pass_time()), sign);
		}

		long long get_pass_time() const
		{
			auto end_time = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
			return duration / dvd;
		}

	private:
		std::chrono::steady_clock::time_point start_time;

		size_t dvd = 1;
	};
}