#pragma once
#include <cstdint>
#include <type_traits>
#include <cassert>


namespace ayr
{
	class Ayr {};

	// container size type 
	using c_size = int64_t;
	// compare type
	using cmp_t = int64_t;

	constexpr char __NONE__ = '\0';

	// 空值
	template<typename T>
	T None = static_cast<T>(*reinterpret_cast<std::remove_reference_t<T>*>(const_cast<char*>(&__NONE__)));

	// a^b % MOD快速幂
	inline size_t qmi(size_t a, size_t b, size_t MOD = LLONG_MAX) noexcept
	{
		assert(MOD > 0);

		a %= MOD;
		size_t res = 1;
		while (b)
		{
			if (b & 1) res = (res * a) % MOD;
			b >>= 1;
			a = a * a % MOD;
		}

		return res;
	}
}