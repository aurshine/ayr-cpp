#ifndef AYR_BASE_AYR_TRAITS_HPP
#define AYR_BASE_AYR_TRAITS_HPP

#include <type_traits>


namespace ayr
{
	// 判断Args的所有类型是否都与T相同
	template<typename... Args>
	struct IsSame : std::false_type {};

	template<typename T1, typename... Args>
	struct IsSame<T1, Args...> : std::bool_constant<(IsSame<T1, Args>::value || ...)> {};

	template<typename T1, typename Arg>
	struct IsSame<T1, Arg> : std::bool_constant<std::is_same_v<std::remove_cvref_t<T1>, std::remove_cvref_t<Arg>>> {};

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

	// 用于判断是否为void，如果是void则返回void，否则返回T&
	template<typename T>
	struct void_or_ref
	{
		using type = T&;
	};

	template<>
	struct void_or_ref<void>
	{
		using type = void;
	};

	template<typename T>
	using void_or_ref_t = typename void_or_ref<T>::type;
}

#endif