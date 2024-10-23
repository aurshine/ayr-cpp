#ifndef AYR_DETAIL_AYR_TRAITS_HPP
#define AYR_DETAIL_AYR_TRAITS_HPP

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


	// 判断是否是char数组
	template <typename T>
	struct IsCharArray : std::false_type {};

	template <std::size_t N>
	struct IsCharArray<char[N]> : std::true_type {};

	template <std::size_t N>
	struct IsCharArray<char(&)[N]> : std::true_type {};

	template <typename T>
	constexpr bool ischararray = IsCharArray<T>::value;

	template<bool ...B>
	constexpr bool And = (B && ...);

	template<bool ...B>
	constexpr bool Or = (B || ...);

	template<bool B>
	constexpr bool Not = !B;
}

#endif