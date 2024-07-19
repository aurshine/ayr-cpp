#pragma once
#include <law/detail/printer.hpp>
#include <law/detail/Array.hpp>

namespace ayr
{
	constexpr uint64_t exp2(int i) { return 1ull << i; }


	template<typename B>
	inline B lowbit(const B& x)
	{
		return x & -x;
	}


	template<typename B>
	inline int lowbit_index(const B& x)
	{
		int l = 0;
		while ((x >> l & 1) == 0) ++l;
		return l;
	}


	template<typename B>
	inline int highbit_index(const B& x)
	{
		int l = 0;
		while (x >> l) ++l;

		return --l;
	}


	template<typename B>
	inline B highbit(const B& x)
	{
		return x >> highbit_index(x);
	}
}
