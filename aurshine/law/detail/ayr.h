#pragma once
#include <cstdint>
#include <cassert>


namespace ayr
{
	class Ayr {};

	// container size type 
	using c_size = int64_t;
	// compare type
	using cmp_t = int64_t;

	// hash type
	using hash_t = uint64_t;

	constexpr char __NONE__ = '\0';

	// 空值
	template<typename T>
	T None = static_cast<T>(*reinterpret_cast<std::remove_reference_t<T>*>(const_cast<char*>(&__NONE__)));

	template<typename T>
	inline bool is_none(const T& val) { return &val == &None<T>; }

}