#pragma once
#include <string>
#include <sstream>
#include <type_traits>
#include <iostream>
#include <typeinfo>

namespace ayr
{
	class Object
	{
	public:
		// 转换为 string 类型
		virtual std::string to_string() const
		{

			std::stringstream ss;
			ss << "<" << typeid(*this).name() << " 0x" << std::hex << this << ">";
			return ss.str();
		}

		// hash 编码
		virtual size_t hash() const
		{
			return std::hash<std::string>{}(this->to_string());
		}

		// 相等
		virtual bool operator= (const Object& obj) const
		{
			return this == &obj;
		}
	};
}