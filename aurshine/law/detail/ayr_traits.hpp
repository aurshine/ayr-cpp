#pragma once
#include <type_traits>


namespace ayr
{
	// 判断Args的所有类型是否都与T相同
	template<typename T, typename ...Args>
	constexpr bool issame = false;

	template<typename T>
	constexpr bool issame<T, T> = true;

	template<typename T1, typename T2>
	constexpr bool issame<T1, T2> = false;

	template<typename T1, typename T2, typename ...Args>
	constexpr bool issame<T1, T2, Args...> = issame<T1, T2>&& issame<T1, Args...>;


	// 判断Args是否包含类型T
	template<typename T, typename... Args>
	constexpr bool isinstance = false;

	template<typename T>
	constexpr bool isinstance<T, T> = true;

	template<typename T1, typename T2>
	constexpr bool isinstance<T1, T2> = false;

	template<typename T1, typename T2, typename... Args>
	constexpr bool isinstance<T1, T2, Args...> = isinstance<T1, T2> || isinstance<T1, Args...>;
}