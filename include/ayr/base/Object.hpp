#ifndef AYR_DETIAL_OBJECT_HPP
#define AYR_DETIAL_OBJECT_HPP

#include <iosfwd>
#include <utility>

#include "meta/ayr_concepts.hpp"
#include "meta/Buffer.hpp"
#include "meta/CString.hpp"
#include "meta/hash.hpp"
#include "meta/None.hpp"

namespace ayr
{
	template<typename Derived>
	class Object
	{
	public:
		using AyrObjectDerived = Derived;

		Object() = default;

		Object(const Object&) = delete;

		Object(Object&&) = delete;

		Object& operator=(const Object&) = delete;

		Object& operator=(Object&&) = delete;

		Derived& derived() { return static_cast<Derived&>(*this); }

		const Derived& derived() const { return static_cast<const Derived&>(*this); }

		// hash 编码
		hash_t __hash__() const { throw std::runtime_error("not implemented"); return None; }

		// 返回值大于0为大于， 小于0为小于，等于0为等于
		cmp_t __cmp__(const Derived& other) const { return reinterpret_cast<cmp_t>(this) - reinterpret_cast<cmp_t>(&other); }

		// 返回true或false表示是否相等
		bool __equals__(const Derived& other) const { return derived().__cmp__(other) == 0; }

		// 交换两个对象
		void __swap__(Derived& other)
		{
			if constexpr (std::is_swappable_v<Derived>)
				std::swap(derived(), other);
			else
				throw std::runtime_error("not implemented");
		}
	};

	template<typename T>
	concept AyrObject = hasattr(T, AyrObjectDerived) && isinstance<T, Object<typename T::AyrObjectDerived>>;

	template<AyrObject T>
	std::ostream& operator<<(std::ostream& os, const T& obj)
	{
		os << obj.__str__().data();
		return os;
	}

	template<typename T> requires(Not<Printable<T>>)
		std::ostream& operator<<(std::ostream& os, const T& obj)
	{
		os << "<" << dtype(T) << " 0x" << std::hex << &obj << ">";
		return os;
	}

	template<AyrObject T, typename U>
	bool operator==(const T& a, const U& b)
	{
		if constexpr (hasmethod(T, __equals__, std::declval<const U&>()))
			return a.__equals__(b);
		return a.__cmp__(b) == 0;
	}

	template<AyrObject T, typename U>
	bool operator!=(const T& a, const U& b)
	{
		if constexpr (hasmethod(T, __equals__, std::declval<const U&>()))
			return !a.__equals__(b);
		return a.__cmp__(b) != 0;
	}

	template<AyrObject T, typename U>
	bool operator>(const T& a, const U& b) { return a.__cmp__(b) > 0; }

	template<AyrObject T, typename U>
	bool operator<(const T& a, const U& b) { return a.__cmp__(b) < 0; }

	template<AyrObject T, typename U>
	bool operator>=(const T& a, const U& b) { return a.__cmp__(b) >= 0; }

	template<AyrObject T, typename U>
	bool operator<=(const T& a, const U& b) { return a.__cmp__(b) <= 0; }

	template<typename T>
	void swap(T& a, T& b)
	{
		if constexpr (hasmethod(T, __swap__, std::declval<T&>()))
			a.__swap__(b);
		else if constexpr (std::swappable<T>)
			std::swap(a, b);
		else
			throw std::runtime_error(std::format("type {} is not swappable", dtype(T)));
	}
}
#endif