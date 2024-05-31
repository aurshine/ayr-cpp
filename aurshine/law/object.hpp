#pragma once
#include <format>
#include <sstream>
#include <typeinfo>

namespace ayr
{
	// container size type 
	using c_size = int64_t;
	// compare type
	using cmp_t = int64_t;

	// __str__ 方法返回值的缓存最大长度
	constexpr static const size_t __STR_BUFFER_SIZE__ = 128;
	// __str__ 方法返回值的缓存
	static char __str__buffer__[__STR_BUFFER_SIZE__]{};


	class Object
	{
	public:
		// 转换为 字符串 类型
		virtual const char* __str__() const
		{
			std::stringstream addr;
			addr << std::hex << this;

			auto&& str = std::format(R"(<{} 0x{}>)", typeid(*this).name(), addr.str());
			std::memcpy(__str__buffer__, str.c_str(), 
				std::min(sizeof(decltype(__str__buffer__)) * str.size(),
						sizeof(decltype(__str__buffer__)) * __STR_BUFFER_SIZE__)
			);
			return __str__buffer__;
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