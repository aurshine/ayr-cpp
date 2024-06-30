#pragma once
#include <cstdint>
#include <type_traits>


namespace ayr
{
	class Ayr {};

	// container size type 
	using c_size = int64_t;
	// compare type
	using cmp_t = int64_t;

	// 空值
	template<typename T>
	T None = static_cast<T>(*reinterpret_cast<std::remove_reference_t<T>*>(nullptr));
}