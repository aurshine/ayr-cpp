#ifndef AYR_BASE_AYR_TRAITS_HPP
#define AYR_BASE_AYR_TRAITS_HPP

#include <type_traits>

#include "ayr.h"

namespace ayr
{
	// 判断Args的所有类型是否都与T相同
	template<typename... Args>
	struct IsSame : std::false_type {};

	template<typename T1, typename... Args>
	struct IsSame<T1, Args...> : std::bool_constant<(IsSame<T1, Args>::value || ...)> {};

	template<typename T1, typename Arg>
	struct IsSame<T1, Arg> : std::bool_constant<std::is_same_v<std::decay_t<T1>, std::decay_t<Arg>>> {};

	template<typename T, typename... Args>
	constexpr bool issame = IsSame<T, Args...>::value;


	// 判断T是否是Args的子类
	template<typename... Args>
	struct IsInstance : std::false_type {};

	template<typename T1, typename... Args>
	struct IsInstance<T1, Args...> : std::bool_constant<(IsInstance<T1, Args>::value || ...)> {};

	template<typename T1, typename Arg>
	struct IsInstance<T1, Arg> : std::bool_constant<std::is_base_of_v<Arg, T1> || issame<T1, Arg>> {};

	template<typename T, typename... Args>
	constexpr bool isinstance = IsInstance<T, Args...>::value;

	// 逻辑运算符
	template<bool ...B>
	constexpr bool And = (B && ...);

	template<bool ...B>
	constexpr bool Or = (B || ...);

	template<bool B>
	constexpr bool Not = !B;

	// 可迭代对象对内部迭代器类型萃取
	template<typename T>
	struct inner_iterator_type : std::type_identity<decltype(std::declval<T>().begin())> {};

	template<typename T>
	struct inner_iterator_type<T*> : std::type_identity<T*> {};

	template<typename T>
	struct inner_iterator_type<T[]> : std::type_identity<T*> {};

	template<typename T, std::size_t N>
	struct inner_iterator_type<T[N]> : std::type_identity<T*> {};

	template<typename T>
		requires hasattr(T, iterator)
	struct inner_iterator_type<T> : std::type_identity<typename T::iterator> {};

	template<typename T>
		requires hasattr(T, Iterator)
	struct inner_iterator_type<T> : std::type_identity<typename T::Iterator> {};

	template<typename T>
	using traits_inner_iterator_t = typename inner_iterator_type<T>::type;

	// 类型添加const修饰
	template<bool cond, typename T>
	struct AddConstIf : std::type_identity<T> {};

	template<typename T>
	struct AddConstIf<true, T> : std::type_identity<const T> {};

	template<bool cond, typename T>
	using add_const_t = typename AddConstIf<cond, T>::type;

	// 万能引用时，根据T1类型转发T2类型
	// T1为左值则T2转发为左值
	// T1为右值则T2转发为右值
	template<typename T1, typename T2>
		requires std::is_lvalue_reference_v<T1>
	def cond_forward(T2&& arg) -> T2&&
	{
		return static_cast<T2&&>(arg);
	}

	template<typename T1, typename T2>
	def cond_forward(T2&& arg) -> std::remove_reference_t<T2>
	{
		static_assert(std::is_lvalue_reference_v<T2>, "bad forward call");
		return static_cast<std::remove_reference_t<T2>>(arg);
	}
}

#endif