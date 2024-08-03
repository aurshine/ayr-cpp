#pragma once
#include <cstdint>
#include <cassert>


namespace ayr
{
#define def inline auto

#define der(R) inline R

#define dtype(T) typeid(T).name()

#define neg_index(index, size) (((index) == (size)) ? (index): (((index) + (size)) % (size)))

#define ifelse(expr, t, f) ((expr)? (t) : (f))

	class Ayr {};

	// container size type 
	using c_size = int64_t;
	// compare type
	using cmp_t = int64_t;

	// hash type
	using hash_t = uint64_t;

	constexpr char __NONE__[2] = "\0";

	// 空值
	template<typename T>
	T& None = *reinterpret_cast<T*>(const_cast<char*>(__NONE__));

	template<typename T>
	def is_none(const T& val) -> bool { return &val == &None<T>; }
}