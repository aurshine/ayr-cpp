#ifndef AYR_BASE_META_AYR_CONCEPTS_HPP
#define AYR_BASE_META_AYR_CONCEPTS_HPP

#include <iostream>
#include <concepts>
#include <ranges>

#include "ayr_traits.hpp"


namespace ayr
{
	template<typename T1, typename T2>
	concept DecaySameAs = std::same_as<std::decay_t<T1>, std::decay_t<T2>>;

	template<typename T1, typename T2>
	concept DecayConvertibleTo = std::convertible_to<std::decay_t<T1>, std::decay_t<T2>>;

	// 基础类型约束概念, 没有析构函数的类型
	template<typename T>
	concept BaseType = Or<
		std::is_fundamental_v<T>, // 算术类型 void 或 nullptr_t
		std::is_pointer_v<T>, // 指针类型
		std::is_array_v<T>, // 数组类型
		std::is_enum_v<T>, // 枚举类型
		std::is_union_v<T>, // 联合类型
		std::is_function_v<T>, // 函数类型
		std::is_reference_v<T> // 引用类型
	>;

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
		{ std::hash<std::decay_t<T>>()(one) } -> std::convertible_to<hash_t>;
		{ one == other } -> std::convertible_to<bool>;
	};

	template<typename T>
	concept HashableImpl = AyrLikeHashable<T> || StdHashable<T>;

	template<typename T>
	concept Hashable = HashableImpl<std::decay_t<T>>;

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

	// 继承后表示释放内存不需要析构
	struct NoDestroyObj {};

	// 释放内存不需要析构概念
	template<typename T>
	concept NoDestroy = Or<BaseType<T>, isinstance<T, NoDestroyObj>>;
}
#endif // AYR_BASE_META_AYR_CONCEPTS_HPP