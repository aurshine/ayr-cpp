#ifndef AYR_LAW_DETAIL_AYR_CONCEPTS_HPP
#define AYR_LAW_DETAIL_AYR_CONCEPTS_HPP

#include <iostream>
#include <concepts>
#include <ranges>

#include <law/detail/ayr.h>
#include <law/detail/ayr_traits.hpp>


namespace ayr
{
	// 判断T是否为char类型
	template<typename T>
	concept Char = issame<T, char, wchar_t, char8_t, char16_t, char32_t>;

	// 可以隐式转换为const char*类型约束概念
	template<typename S>
	concept ConveribleToCstr = std::convertible_to<S, const char*>;


	// 可输出的类型约束概念
	template<typename T>
	concept StdPrintable = issame<T,
		bool,
		char,
		int,
		unsigned int,
		long,
		unsigned long,
		long long,
		unsigned long long,
		float,
		double,
		long double,
		nullptr_t,
		std::string> || std::is_pointer_v<T> || ischararray<T>;

	template<typename T>
	concept AyrPrintable = requires(T t)
	{
		{ t.__str__() } -> ConveribleToCstr;
	};

	template<typename T>
	concept Printable = AyrPrintable<T> || StdPrintable<T>;

	// 可哈希类型约束概念
	template<typename T>
	concept AyrLikeHashable = requires(const T & one, const T & other)
	{
		{ one.__hash__() } -> std::convertible_to<hash_t>;
		{ one.__cmp__(other) } -> std::convertible_to<cmp_t>;
	};


	template<typename T>
	concept StdHashable = requires(const T & one, const T & other)
	{
		{ std::hash<T>()(one) } -> std::convertible_to<hash_t>;
		{ one == other } -> std::convertible_to<bool>;
	};

	template<typename T>
	concept Hashable = AyrLikeHashable<T> || StdHashable<T>;
}
#endif