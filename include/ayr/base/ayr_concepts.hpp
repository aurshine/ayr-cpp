#ifndef AYR_BASE_AYR_CONCEPTS_HPP
#define AYR_BASE_AYR_CONCEPTS_HPP

#include <iostream>
#include <concepts>
#include <ranges>

#include "ayr.h"
#include "ayr_traits.hpp"


namespace ayr
{
	template<typename T1, typename T2>
	concept DecaySameAs = std::same_as<std::decay_t<T1>, std::decay_t<T2>>;

	template<typename T1, typename T2>
	concept DecayConvertibleTo = std::convertible_to<std::decay_t<T1>, std::decay_t<T2>>;

	// 可以隐式转换为const char*类型约束概念
	template<typename S>
	concept ConveribleToCstr = std::convertible_to<S, const char*>;

	// 可输出的类型约束概念
	template<typename T>
	concept StdPrintable = Or<
		std::is_pointer_v<T>,
		std::is_integral_v<T>,
		std::is_floating_point_v<T>,
		issame<T, char*, nullptr_t, std::string>>;

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
	concept HashableImpl = AyrLikeHashable<T> || StdHashable<T>;

	template<typename T>
	concept Hashable = HashableImpl<std::remove_cvref_t<T>>;

	// 迭代器约束, 迭代器的解引用是T
	template<typename It, typename V>
	concept IteratorVImpl = requires(It it) { { *it } -> DecaySameAs<V>; };

	template<typename It, typename V>
	concept IteratorV = Or<
		std::same_as<std::remove_pointer_t<It>, V>,
		And<std::input_or_output_iterator<It>, IteratorVImpl<It, V>>
	>;

	// 迭代器约束，迭代器的解引用可隐式转化为U
	template<typename It, typename U>
	concept IteratorUImpl = requires(It it) { { *it } -> DecayConvertibleTo<U>; };

	template<typename It, typename U>
	concept IteratorU = Or<
		std::same_as<std::remove_pointer_t<It>, U>,
		And<std::input_or_output_iterator<It>, IteratorUImpl<It, U>>
	>;

	// 可迭代类型约束概念
	template<typename Obj>
	concept Iteratable = requires(Obj obj)
	{
		{ obj.begin() } -> std::input_or_output_iterator;
		{ obj.end() } -> std::input_or_output_iterator;
	};

	// 可迭代类型约束概念, 迭代的成员是V类型
	template<typename Obj, typename V>
	concept IteratableV = requires(Obj obj)
	{
		{ obj.begin() } -> IteratorV<V>;
		{ obj.end() } -> IteratorV<V>;
	};

	// 可迭代类型约束概念， 迭代的成员可隐式转化为U类型
	template<typename Obj, typename U>
	concept IteratableU = requires(Obj obj)
	{
		{ obj.begin() } -> IteratorU<U>;
		{ obj.end() } -> IteratorU<U>;
	};
}
#endif