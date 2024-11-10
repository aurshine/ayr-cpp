#ifndef AYR_DETIAL_OBJECT_HPP
#define AYR_DETIAL_OBJECT_HPP

#include <sstream>

#include <ayr/detail/CString.hpp>
#include <ayr/detail/ayr_concepts.hpp>


namespace ayr
{
	template<typename Derived>
	class Object
	{
		using self = Derived;
	public:
		using AyrObjectDerived = Derived;

		// 转换为 字符串 类型
		CString __str__() const
		{
			std::string type_name = dtype(self);
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
		hash_t __hash__() const { assert(false, "not implemented __hash__()"); return None<hash_t>; }

		// 返回值大于0为大于， 小于0为小于，等于0为等于
		cmp_t __cmp__(const self& other) const { return reinterpret_cast<cmp_t>(this) - reinterpret_cast<cmp_t>(&other); }

		// 返回true或false表示是否相等
		bool __equals__(const self& other) const { return __cmp__(other) == 0; }
	};

	template<typename T>
	concept AyrObject = requires(T & t)
	{
		typename T::AyrObjectDerived;

	}&& isinstance<T, Object<typename T::AyrObjectDerived>>;

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

	template<AyrObject T>
	bool operator==(const T& a, const T& b) { return a.__equals__(b); }

	template<AyrObject T>
	bool operator!=(const T& a, const T& b) { return !a.__equals__(b); }

	template<AyrObject T>
	bool operator>(const T& a, const T& b) { return a.__cmp__(b) > 0; }

	template<AyrObject T>
	bool operator<(const T& a, const T& b) { return a.__cmp__(b) < 0; }

	template<AyrObject T>
	bool operator>=(const T& a, const T& b) { return a.__cmp__(b) >= 0; }

	template<AyrObject T>
	bool operator<=(const T& a, const T& b) { return a.__cmp__(b) <= 0; }
}
#endif