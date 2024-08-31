#ifndef AYR_LAW_DETIAL_OBJECT_HPP
#define AYR_LAW_DETIAL_OBJECT_HPP

#include <format>
#include <sstream>
#include <typeinfo>

#include <law/detail/CString.hpp>
#include <law/detail/ayr_concepts.hpp>


namespace ayr
{
	template<typename Derived>
	class Object : public Ayr
	{
		using self = Derived;
	public:
		// 转换为 字符串 类型
		virtual CString __str__() const
		{
			std::stringstream addr;
			addr << std::hex << this;

			auto&& str = std::format(R"(<{} 0x{}>)", dtype(Derived), addr.str());

			return CString(str.c_str());
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

}
#endif