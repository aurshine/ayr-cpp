#pragma once
#include <concepts>
#include <ranges>

#include <law/ayr_traits.hpp>


namespace ayr
{
	// �ж�T�Ƿ�Ϊchar����
	template<typename T>
	concept Char = is_same<std::remove_cvref_t<T>, char, wchar_t, char8_t, char16_t, char32_t>;

	// �ж�T�Ƿ�Ϊ�ɵ�������
	template<typename T>
	concept Iteratable = std::ranges::range<T>;

	// �ж�T�Ƿ�Ϊ���βεĿɵ�������
	template<typename T>
	concept Callable = requires(T t)
	{
		{ t() };
	};

	template<typename T>
	concept Hashable = requires(T t1, T t2)
	{
		{ std::hash<T>()(t1) };
		{t1 == t2};
	};
}