﻿#ifndef AYR_DETIAL_OBJECT_HPP
#define AYR_DETIAL_OBJECT_HPP

#include <sstream>

#include "CString.hpp"
#include "ayr_concepts.hpp"


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

		// 转换为 字符串 类型
		CString __str__() const
		{
			std::string type_name = dtype(Derived);
			int s_len = type_name.size() + 22;
			CString s(s_len);
#ifdef _MSC_VER
			sprintf_s(s.data(), s_len, "<%s 0x%p>", type_name.c_str(), this);
#else
			std::sprintf(s.data(), "<%s %p>", type_name.c_str(), this);
#endif
			return s;
		}

		// hash 编码
		hash_t __hash__() const { throw std::runtime_error("not implemented"); return None<hash_t>; }

		// 返回值大于0为大于， 小于0为小于，等于0为等于
		cmp_t __cmp__(const Derived& other) const { return reinterpret_cast<cmp_t>(this) - reinterpret_cast<cmp_t>(&other); }

		// 返回true或false表示是否相等
		bool __equals__(const Derived& other) const { return derived().__cmp__(other) == 0; }
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
	bool operator==(const T& a, const U& b) { return a.__equals__(b); }

	template<AyrObject T, typename U>
	bool operator!=(const T& a, const U& b) { return !a.__equals__(b); }

	template<AyrObject T, typename U>
	bool operator>(const T& a, const U& b) { return a.__cmp__(b) > 0; }

	template<AyrObject T, typename U>
	bool operator<(const T& a, const U& b) { return a.__cmp__(b) < 0; }

	template<AyrObject T, typename U>
	bool operator>=(const T& a, const T& b) { return a.__cmp__(b) >= 0; }

	template<AyrObject T, typename U>
	bool operator<=(const T& a, const U& b) { return a.__cmp__(b) <= 0; }
}
#endif