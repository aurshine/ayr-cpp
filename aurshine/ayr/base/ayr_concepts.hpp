#ifndef AYR_DETAIL_AYR_CONCEPTS_HPP
#define AYR_DETAIL_AYR_CONCEPTS_HPP

#include <iostream>
#include <concepts>
#include <ranges>

#include <ayr/base/ayr.h>
#include <ayr/base/ayr_traits.hpp>


namespace ayr
{
	// 可以隐式转换为const char*类型约束概念
	template<typename S>
	concept ConveribleToCstr = std::convertible_to<S, const char*>;


	// 可输出的类型约束概念
	template<typename T>
	concept StdPrintable = std::is_pointer_v<T> || issame<std::decay_t<T>, char*> || issame<T,
		bool,
		char,
		short,
		unsigned short,
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
		std::string>;

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

	// 可迭代类型约束概念
	template<typename T>
	concept Iteratable = requires(T obj)
	{
		{ obj.begin() };
		{ obj.end() };
		{ obj.size() };
	};
}
#endif