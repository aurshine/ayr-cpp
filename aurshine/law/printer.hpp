#pragma once
#include <iostream>
#include <format>
#include <mutex>

#include <law/object.hpp>

namespace ayr
{
	template<class Ostream>
	class Printer : public Object
	{
	public:
		Printer(Ostream* ostream): Printer("\n", " ", ostream) {}

		template<class T1, class T2>
		Printer(T1&& end_word, T2&& sep_word, Ostream* ostream)
			: end_word(std::forward<T1>(end_word)),
			sep_word(std::forward<T2>(sep_word)),
			ostream(ostream){}

		// 可变形参
		template<class T, class ...Args>
		void operator() (const T& object, const Args& ...args)
		{
			std::lock_guard<std::recursive_mutex> lock(this->mutex);
			*ostream << object << " ";
			this->operator()(args...);
		}

		// 单一形参
		template<class T>
		void operator() (const T& object)
		{
			std::lock_guard<std::recursive_mutex> lock(this->mutex);
			*ostream << object << this->end_word;
		}

		// 设置输出结束符
		template<class T>
		void set_end_word(T&& end_word)
		{
			std::lock_guard<std::recursive_mutex> lock(this->mutex);
			this->end_word = std::forward<T>(end_word);
		}

		// 设置输出分隔符
		template<class T>
		void set_sep_word(T&& end_word)
		{
			std::lock_guard<std::recursive_mutex> lock(this->mutex);
			this->sep_word = std::forward<T>(end_word);
		}

	private:
		std::string end_word;

		std::string sep_word;

		std::recursive_mutex mutex;

		Ostream* ostream;
	};


	class Color : Object
	{
	public:
		constexpr static const char* CLOSE = "\033[0m";

		constexpr static const char* BLACK = "\033[30m";

		constexpr static const char* RED = "\033[31m";

		constexpr static const char* GREEN = "\033[32m";

		constexpr static const char* YELLOW = "\033[33m";

		constexpr static const char* BLUE = "\033[34m";

		constexpr static const char* PURPLE = "\033[35m";

		constexpr static const char* DEEPGREEN = "\033[36m";

		constexpr static const char* WHITE = "\033[37m";
	};


	template<class Ostream>
	class ColorPrinter : public Printer<Ostream>
	{
	public:
		ColorPrinter(Ostream* ostream, const char* color)
			: Printer<Ostream>("", " ", ostream)
		{
			this->operator()(color);
			this->set_end_word("\n");
		}

		~ColorPrinter()
		{
			this->set_end_word("");
			this->operator()(Color::CLOSE);
		}
	};

	static Printer<std::ostream> print(&std::cout);

	// 标准输出流 黄色 打印
#define ayr_warner ColorPrinter<std::ostream>(&std::cout, ayr::Color::YELLOW)
	// 标准输出流 红色 打印
#define ayr_error ColorPrinter<std::ostream>(&std::cout, ayr::Color::RED)


	inline void _warn_assert(bool condition, const std::string& msg)
	{
		{
			if (!condition)
				ayr_warner(msg);
		}
	}


	inline void _error_assert(bool condition, const std::string& msg)
	{
		if (!condition)
		{
			ayr_error(msg);
			exit(-1);
		}
	}

	// 打印警告信息,继续向下执行
#define warn_assert(condition, info) _warn_assert(condition, std::format("{} [{}]\nINFORMATION: {}", __FILE__, __LINE__, info))
	// 打印错误信息，并退出
#define error_assert(condition, info) _error_assert(condition, std::format("{} [{}]\nINFORMATION: {}", __FILE__, __LINE__, info))
}
