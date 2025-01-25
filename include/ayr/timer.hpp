#ifndef AYR_TIMER_HPP
#define AYR_TIMER_HPP

#include <ctime>
#include <chrono>

#include "base/raise_error.hpp"

namespace ayr
{
	class Date : public Object<Date>
	{
	public:
		Date() : Date(std::time(nullptr)) {}

		Date(time_t second_from_19700101)
		{
			year_ = 1970, month_ = 1, day_ = 1;
			time_t day_from_19700101 = second_from_19700101 / (24 * 3600);
			time_t sec_from_19700101 = second_from_19700101 % (24 * 3600);

			int TIMEZONE = 8; // 时区转换，北京时间比UTC时间早八个小时
			hour_ = TIMEZONE + sec_from_19700101 / 3600;
			minute_ = (sec_from_19700101 % 3600) / 60;
			second_ = sec_from_19700101 % 60;
			week_ = (WEEK_DAY_FROM_1970_01_01 + day_from_19700101) % 7;
			while (day_from_19700101)
			{
				int is_run = (check_run_year(year_) ? 1 : 0);
				int y = 365 + is_run;
				int m = MONTHS[month_] + (month_ == 2 ? is_run : 0);

				if (day_from_19700101 - y >= 0)
				{
					year_++;
					day_from_19700101 -= y;
				}
				else if (day_from_19700101 - m >= 0)
				{
					month_++;
					day_from_19700101 -= m;
				}
				else
				{
					day_ += day_from_19700101;
					day_from_19700101 = 0;
				}
			}
		}

		Date(int year, int month, int day, int hour, int minute, int second)
			: year_(year), month_(month), day_(day), week_(calc_week(year, month, day)), hour_(hour), minute_(minute), second_(second) {}

		int year() const { return year_; }

		int month() const { return month_; }

		int day() const { return day_; }

		int week() const { return week_; }

		// 返回当前是周几
		CString week_str() const { return WEEK_STR[week()]; }

		CString __str__() const { return std::format("{} {:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}", WEEK_STR[week_], year_, month_, day_, hour_, minute_, second_); }

		cmp_t __cmp__(const Date& date) const
		{
			if (year_ != date.year_) return year_ - date.year_;
			if (month_ != date.month_) return month_ - date.month_;
			return day_ - date.day_;
		}

		bool __equals__(const Date& date) const { return __cmp__(date) == 0; }

		// 计算星期
		static int calc_week(int year, int month, int day)
		{
			return (day + 2 * month + 3 * (month + 1) / 5 + year + year / 4 - year / 100 + year / 400 + 1) % 7;
		}

		// 判断闰年
		static bool check_run_year(int year) { return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0); }

		// 1970-01-01是星期四，星期天为0
		constexpr static const int WEEK_DAY_FROM_1970_01_01 = 4;

		// 每月天数
		constexpr static int MONTHS[13]{ 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

		// 星期字符串
		constexpr static const char* WEEK_STR[7]{ "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

	private:
		int year_, month_, day_, week_, hour_, minute_, second_;
	};


	template<typename Duration>
	class Timer : public Object<Timer<Duration>>
	{
	public:
		void into() { start_time = std::chrono::steady_clock::now(); }

		typename Duration::rep escape()
		{
			auto end_time = std::chrono::steady_clock::now();
			return std::chrono::duration_cast<Duration>(end_time - start_time).count();;
		}

		template<typename F, typename ...Args>
		typename Duration::rep operator()(F&& call_, Args&&... args)
		{
			into();
			call_(std::forward<Args>(args)...);
			return escape();
		}
	private:
		std::chrono::steady_clock::time_point start_time;
	};

	using Timer_s = Timer<std::chrono::seconds>;
	using Timer_ms = Timer<std::chrono::milliseconds>;
	using Timer_us = Timer<std::chrono::microseconds>;
	using Timer_ns = Timer<std::chrono::nanoseconds>;
}
#endif