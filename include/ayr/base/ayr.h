#ifndef AYR_BASE_AYR_H
#define AYR_BASE_AYR_H

#include <cstdint>
#include <cassert>
#include <typeinfo>

namespace ayr
{
#define def inline auto

#define der(R) inline R

#define dtype(T) typeid(T).name()

	// 取模运算, 运算结果为 [0, size]
#define neg_index(index, size) (((index) == (size)) ? (index): (((index) + (size)) % (size)))

#define ifelse(expr, t, f) ((expr)? (t) : (f))

#define hastype(T, type) requires(T t) { typename T::type; }

#define hasmember(T, member) requires(T t) { &T::member; }

#define hasmethod(T, method, ...) requires(T t) { t.method( ##__VA_ARGS__);}

#define hasattr(T, attr) hasmember(T, attr) || hastype(T, attr)

	// container size type 
	using c_size = int64_t;
	// compare type
	using cmp_t = int64_t;

	// hash type
	using hash_t = uint64_t;

	// 空值, 不会被使用, 仅用于占位符
	template<typename T>
	T& None = *reinterpret_cast<T*>(nullptr);
}

#endif