#pragma once
#include <format>
#include <sstream>
#include <typeinfo>

#include <law/_ayr.h>
#include <law/_str_buffer.hpp>


namespace ayr
{
	class Object: public Ayr
	{
	public:
		// 转换为 字符串 类型
		const char* __str__() const
		{
			std::stringstream addr;
			addr << std::hex << this;

			auto&& str = std::format(R"(<{} 0x{}>)", typeid(*this).name(), addr.str());
			memcpy__str_buffer__(str.c_str(), str.size());

			return __str_buffer__;
		}

		// hash 编码
		size_t __hash__() const { return std::hash<std::string>{}(this->__str__()); }

		// 返回值大于0为大于， 小于0为小于，等于0为等于
		cmp_t __cmp__(const Object& other) const { return (cmp_t)this - (cmp_t)&other; }
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


template<ayr::DerivedAyr T>
struct std::hash<T>
{
	size_t operator()(const T& one) const { return one.__hash__(); }
};