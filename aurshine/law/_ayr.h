#pragma once
#include <cstdint>
#include <type_traits>
#include <iterator>
#include <concepts>

namespace ayr
{
	class Ayr {};

	// container size type 
	using c_size = int64_t;
	// compare type
	using cmp_t = int64_t;

	template<typename T>
	concept Char = std::is_same_v<char, std::remove_cvref_t<T>>
		|| std::is_same_v<wchar_t, std::remove_cvref_t<T>>
		|| std::is_same_v<char8_t, std::remove_cvref_t<T>>
		|| std::is_same_v<char16_t, std::remove_cvref_t<T>>
		|| std::is_same_v<char32_t, std::remove_cvref_t<T>>;

	template<typename T>
	concept Iteratable = std::ranges::range<T>;
}