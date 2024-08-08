#ifndef AYR_LAW_DETIAL_BUNIT_HPP
#define AYR_LAW_DETIAL_BUNIT_HPP

namespace ayr
{
	constexpr inline uint64_t exp2(int i) { return 1ull << i; }


	template<typename B>
	constexpr inline B lowbit(const B& x)
	{
		return x & -x;
	}


	template<typename B>
	constexpr inline int lowbit_index(const B& x)
	{
		int l = 0;
		while ((x >> l & 1) == 0) ++l;
		return l;
	}


	template<typename B>
	constexpr inline int highbit_index(const B& x)
	{
		int l = 0;
		while (x >> l) ++l;

		return --l;
	}


	template<typename B>
	constexpr inline B highbit(const B& x) { return x >> highbit_index(x); }

	template<typename B>
	constexpr inline bool all_one(const B& x) { return (x & x + 1) == 0; }

	template<typename B>
	constexpr inline bool only_one(const B& x) { return (x & x - 1) == 0; }
}
#endif