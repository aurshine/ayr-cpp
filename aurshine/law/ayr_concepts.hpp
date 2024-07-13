#pragma once
#include <concepts>
#include <ranges>

#include <law/ayr_traits.hpp>


namespace ayr
{
	// 判断T是否为char类型
	template<typename T>
	concept Char = isinstance<std::remove_cvref_t<T>, char, wchar_t, char8_t, char16_t, char32_t>;

	// 判断T是否为可迭代类型
	template<typename T>
	concept Iteratable = std::ranges::range<T>;

	// 判断T是否为无形参的可调用类型
	template<typename T>
	concept Callable = requires(T t)
	{
		{ t() };
	};


	// 可输出的类型约束概念
	template<typename T>
	concept StdPrintable = requires(T t)
	{
		{ std::cout << t };
	};

	template<typename T>
	concept AyrPrintable = requires(T t)
	{
		{ t.__str__() };
	};

	template<typename T>
	concept Printable = AyrPrintable<T> || StdPrintable<T>;

	// 可哈希类型约束概念
	template<typename T>
	concept AyrLikeHashable = requires(const T & one, const T & other)
	{
		{ one.__hash__() } -> std::convertible_to<size_t>;
		{ one.__cmp__(other) } -> std::convertible_to<cmp_t>;
	};


	template<typename T>
	concept StdHashable = requires(const T & one, const T & other)
	{
		{ std::hash<T>()(one) } -> std::convertible_to<size_t>;
		{ one == other } -> std::convertible_to<bool>;
	};

	template<typename T>
	concept Hashable = AyrLikeHashable<T> || StdHashable<T>;
}