#ifndef AYR_LAW_TIMER_HPP
#define AYR_LAW_TIMER_HPP

#include <ctime>
#include <chrono>

#include <law/Printer.hpp>
#include <law/Array.hpp>
#include <law/Wrapper.hpp>

namespace ayr
{
	class Date : public Object
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

		CString week_str() const { return WEEK_STR[week_]; }

		CString __str__() const { return std::format("{} {:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}", WEEK_STR[week_], year_, month_, day_, hour_, minute_, second_); }
		
		cmp_t __cmp__(const Date& date) const
		{
			return Array<int>{year_, month_, day_}.__cmp__(Array<int>{date.year(), date.month(), date.day()});
		}

		static int calc_week(int year, int month, int day)
		{
			return (day + 2 * month + 3 * (month + 1) / 5 + year + year / 4 - year / 100 + year / 400 + 1) % 7;
		}

		static bool check_run_year(int year) { return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0); }

		constexpr static const int WEEK_DAY_FROM_1970_01_01 = 4;

		constexpr static int MONTHS[13]{ 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

		constexpr static const char* WEEK_STR[7]{ "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

	private:
		int year_, month_, day_, week_, hour_, minute_, second_;
	};


	class Timer : public Wrapper
	{
	public:
		Timer() : dvd(100) {};

		Timer(const CString& sec_option) : dvd(0)
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


		void start() override
		{
			start_time = std::chrono::high_resolution_clock::now();
		}


		void stop() override
		{
			CString sign = "";
			if (dvd == 1000)
				sign = "ms";
			else if (dvd == 1000000)
				sign = "s";
			else if (dvd == 1)
				sign = "us";
			print(std::format("pass time: {:.2f}", get_pass_time()), sign);
		}

		double get_pass_time() const
		{
			auto end_time = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
			return 1.0 * duration / dvd;
		}

	private:
		std::chrono::steady_clock::time_point start_time;

		long long dvd;
	};
}
#endif