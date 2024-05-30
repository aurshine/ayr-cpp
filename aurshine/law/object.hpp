#pragma once
#include <format>
#include <sstream>
#include <typeinfo>

namespace ayr
{
	using c_size = int64_t;
	using cmp_t = int64_t;


	class Object
	{
	public:
		// 转换为 字符串 类型
		virtual std::string __str__() const
		{
			std::stringstream stream;
			stream << std::hex << this;

			return std::format(R"(<{} 0x{}>)", typeid(*this).name(), stream.str());
		}

		// hash 编码
		virtual size_t hash() const { return std::hash<std::string>{}(this->__str__()); }

		// 返回值大于0为大于， 小于0为小于，等于0为等于
		virtual cmp_t __cmp__(const Object& other) const { return (cmp_t)this - (cmp_t)&other; }
	};


	template<typename T>
	concept DerivedObject = std::is_base_of_v<Object, T>;


	template<DerivedObject T>
	std::ostream& operator<< (std::ostream& cout, const T& item) { return cout << item.__str__(); }


	template<DerivedObject T1, typename T2>
	bool operator> (const T1& one, const T2& other) { return one.__cmp__(other) > 0; }


	template<DerivedObject T1, typename T2>
	bool operator< (const T1& one, const T2& other) { return one.__cmp__(other) < 0; }


	template<DerivedObject T1, typename T2>
	bool operator>= (const T1& one, const T2& other) { return one.__cmp__(other) >= 0; }


	template<DerivedObject T1, typename T2>
	bool operator<= (const T1& one, const T2& other) { return one.__cmp__(other) <= 0; }


	template<DerivedObject T1, typename T2>
	bool operator== (const T1& one, const T2& other) { return one.__cmp__(other) == 0; }


	template<DerivedObject T1, typename T2>
	bool operator!= (const T1& one, const T2& other) { return one.__cmp__(other) != 0; }
}