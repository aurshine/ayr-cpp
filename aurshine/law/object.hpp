#pragma once
#include <format>
#include <sstream>
#include <typeinfo>

namespace ayr
{
	class Object
	{
	public:
		// 转换为 string 类型
		virtual std::string __str__() const
		{
			std::stringstream stream;
			stream << std::hex << this;

			return std::format(R"(<{} 0x{}>)", typeid(*this).name(), stream.str());
		}

		// hash 编码
		virtual size_t hash() const { return std::hash<std::string>{}(this->__str__()); }


		// 返回值大于0为大于， 小于0为小于，等于0为等于
		virtual int64_t __cmp__(const Object& other) const { return (size_t)this - (size_t)&other; }
	};

	template<typename T>
	concept DerivedObject = std::is_base_of_v<Object, T>;

	template<DerivedObject T>
	std::ostream& operator<< (std::ostream& cout, const T& item)
	{
		cout << item.__str__();
		return cout;
	}

	template<DerivedObject T>
	bool operator> (const T& one, const T& other) { return one.__cmp__(other) > 0; }

	template<DerivedObject T>
	bool operator< (const T& one, const T& other) { return one.__cmp__(other) < 0; }

	template<DerivedObject T>
	bool operator>= (const T& one, const T& other) { return one.__cmp__(other) >= 0; }

	template<DerivedObject T>
	bool operator<= (const T& one, const T& other) { return one.__cmp__(other) <= 0; }

	template<DerivedObject T>
	bool operator== (const T& one, const T& other) { return one.__cmp__(other) == 0; }

	template<DerivedObject T>
	bool operator!= (const T& one, const T& other) { return one.__cmp__(other) != 0; }
}