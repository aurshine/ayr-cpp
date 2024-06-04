#pragma once
#include <format>
#include <string>
#include <chrono>

#include <law/object.hpp>

namespace ayr
{
	// 保存年月日
	struct YearMonthDay { int year, month, day; };

	// 1970_01_01距离现在的天数
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

	class Timer : public Object
	{
	public:
		Timer() = default;

		template<typename F>
		void operator() (const F& func)
		{
			auto start = std::chrono::high_resolution_clock::now();
			func();
			auto end = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
			std::cout << "Time elapsed: " << duration << " microseconds" << std::endl;
		}
	};
}