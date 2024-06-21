#pragma once
#include <type_traits>


namespace ayr
{
	// 判断Args的所有类型是否都与T相同
	template<typename T, typename ...Args>
	constexpr bool is_same = false;

	template<typename T>
	constexpr bool is_same<T, T> = true;

	template<typename T1, typename T2>
	constexpr bool is_same<T1, T2> = false;

	template<typename T1, typename T2, typename ...Args>
	constexpr bool is_same<T1, T2, Args...> = is_same<T1, T2> && is_same<T1, Args...>;
}