#ifndef AYR_LAW_DETIAL_OBJECT_HPP
#define AYR_LAW_DETIAL_OBJECT_HPP

#include <format>
#include <sstream>
#include <typeinfo>

#include <law/detail/CString.hpp>
#include <law/detail/ayr_concepts.hpp>


namespace ayr
{
	class Object : public Ayr
	{
	public:
		// 转换为 字符串 类型
		virtual CString __str__() const
		{
			std::stringstream addr;
			addr << std::hex << this;

			auto&& str = std::format(R"(<{} 0x{}>)", dtype(*this), addr.str());
			
			return CString(str.c_str());
		}

		// hash 编码
		virtual hash_t __hash__() const { assert(false, "not implemented __hash__()"); return None<hash_t>; }

		// 返回值大于0为大于， 小于0为小于，等于0为等于
		virtual cmp_t __cmp__(const Object& other) const { return (cmp_t)this - (cmp_t)&other; }

		virtual bool __equal__(const Object& other) const { return __cmp__(other) == 0; }
	};

	// Ayr 的派生类
	template<typename T>
	concept DerivedAyr = std::is_base_of_v<Ayr, T>;


	template<DerivedAyr T>
	std::ostream& operator<< (std::ostream& cout, const T& item) { return cout << item.__str__(); }


	template<DerivedAyr T1, typename T2>
	bool operator> (const T1& one, const T2& other) { return one.__cmp__(other) > 0; }


	template<DerivedAyr T1, typename T2>
	bool operator< (const T1& one, const T2& other) { return one.__cmp__(other) < 0; }


	template<DerivedAyr T1, typename T2>
	bool operator>= (const T1& one, const T2& other) { return one.__cmp__(other) >= 0; }


	template<DerivedAyr T1, typename T2>
	bool operator<= (const T1& one, const T2& other) { return one.__cmp__(other) <= 0; }


	template<DerivedAyr T1, typename T2>
	bool operator== (const T1& one, const T2& other) { return one.__cmp__(other) == 0; }


	template<DerivedAyr T1, typename T2>
	bool operator!= (const T1& one, const T2& other) { return one.__cmp__(other) != 0; }
}
#endif