#ifndef AYR_DETIAL_OBJECT_HPP
#define AYR_DETIAL_OBJECT_HPP

#include <format>
#include <sstream>
#include <typeinfo>

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
		virtual CString __str__() const
		{
			std::stringstream addr;
			addr << std::hex << this;

			auto&& str = std::format(R"(<{} 0x{}>)", dtype(Derived), addr.str());

			return CString(str);
		}

		// hash 编码
		virtual hash_t __hash__() const { assert(false, "not implemented __hash__()"); return None<hash_t>; }

		// 返回值大于0为大于， 小于0为小于，等于0为等于
		virtual cmp_t __cmp__(const self& other) const { return reinterpret_cast<cmp_t>(this) - reinterpret_cast<cmp_t>(&other); }

		// 返回true或false表示是否相等
		virtual bool __equals__(const self& other) const { return __cmp__(other) == 0; }

		virtual bool operator> (const self& other) const { return __cmp__(other) > 0; }

		virtual bool operator< (const self& other) const { return __cmp__(other) < 0; }

		virtual bool operator>= (const self& other) const { return __cmp__(other) >= 0; }

		virtual bool operator<= (const self& other) const { return __cmp__(other) <= 0; }

		virtual bool operator== (const self& other) const { return __equals__(other); }

		virtual bool operator!= (const self& other) const { return !__equals__(other); }
	};

	template<typename T>
	concept AyrObject = requires(T & t)
	{
		typename T::AyrObjectDerived;

	}&& isinstance<T, Object<typename T::AyrObjectDerived>>;

	template<typename T>
	constexpr bool is_ayr_obj = AyrObject<T>;

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
}
#endif