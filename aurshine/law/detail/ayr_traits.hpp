#ifndef AYR_LAW_DETAIL_AYR_TRAITS_HPP
#define AYR_LAW_DETAIL_AYR_TRAITS_HPP

#include <type_traits>


namespace ayr
{
	// 判断Args的所有类型是否都与T相同
	template<typename T>
	struct _issame
	{
		constexpr bool value = false;
	};
	

	template<typename T1, typename T2, typename ...Args>
	struct _issame
	{
		constexpr bool value = std::is_same_v<T1, T2> || issame<T1, Args...>::value;
	};


	template<typename T1, typename... Args>
	constexpr bool issame = _issame<T1, Args...>::value;

	template<typename T1, typename T2, typename... Args>
	constexpr bool isinstance = std::is_base_of_v<T2, T1> || isinstance<T1, Args...>;

	template<typename T1, typename T2>
	constexpr bool isinstance<T1, T2> = std::is_base_of_v<T2, T1> || std::is_same_v<T1, T2>;
}

#endif