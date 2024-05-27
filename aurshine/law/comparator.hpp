#pragma once
#include "printer.hpp"

namespace ayr
{
	template<class T>
	class Comparator : public Object
	{
	public:
		// 返回值大于0为大于， 小于0为小于，等于0为等于
		virtual int64_t __cmp__(const T& other) const { return (size_t)this - (size_t)&other; }

		bool operator> (const T& other) const { return __cmp__(other) > 0; }

		bool operator< (const T& other) const { return __cmp__(other) < 0; }

		bool operator>= (const T& other) const { return __cmp__(other) >= 0; }

		bool operator<= (const T& other) const { return __cmp__(other) <= 0; }

		bool operator== (const T& other) const { return __cmp__(other) == 0; }

		bool operator!= (const T& other) const { return __cmp__(other) != 0; }
	};
}
