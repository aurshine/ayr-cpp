#ifndef AYR_LAW_DETIAL_BUNIT_HPP
#define AYR_LAW_DETIAL_BUNIT_HPP

namespace ayr
{
	constexpr def exp2(int i) { return 1ull << i; }


	template<typename B>
	constexpr def lowbit(const B& x) { return x & -x; }


	template<typename B>
	constexpr def lowbit_index(const B& x)
	{
		int l = 0;
		while ((x >> l & 1) == 0) ++l;
		return l;
	}


	template<typename B>
	constexpr def highbit_index(const B& x)
	{
		int l = 0;
		while (x >> l) ++l;

		return --l;
	}

	template<typename B>
	constexpr def highbit(const B& x) { return x >> highbit_index(x); }

	// 判断二进制数是否全为1
	template<typename B>
	constexpr def all_one(const B& x) { return (x & x + 1) == 0; }

	// 判断二进制数是否只有一个1
	template<typename B>
	constexpr def only_one(const B& x) { return (x & x - 1) == 0; }

	// 向上取整到2的幂次方
	template<typename B>
	constexpr def roundup2(const B& x)
	{
		if (x <= 1)	return 1;

		--x;
		for (size_t i = 1; i < sizeof(B) * 8; i <<= 1)
			x |= x >> i;

		return x + 1;
	}
}

#endif