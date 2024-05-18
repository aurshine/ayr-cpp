#pragma once
#include "printer.hpp"

namespace ayr
{
	template<class T>
	class Comparator : public Object
	{
	public:
		// > 默认等价于 !__le__
		virtual bool __gt__(const T& other) const { return !__le__(other); }

		// < 默认等价于 !__ge__
		virtual bool __lt__(const T& other) const { return !__ge__(other); }

		// >= 默认等价于 !__lt__
		virtual bool __ge__(const T& other) const { return !__lt__(other); }

		// <= 默认等价于 !__gt__
		virtual bool __le__(const T& other) const { return !__gt__(other); }

		// == 默认等价于 this == &other
		virtual bool __eq__(const T& other) const { return this == &other; }

		// != 默认等价于 !__eq__
		virtual bool __ueq__(const T& other) const { return !__eq__(other); }


		bool operator> (const T& other) const { return __gt__(other); }

		bool operator< (const T& other) const { return __lt__(other); }

		bool operator>= (const T& other) const { return __ge__(other); }

		bool operator<= (const T& other) const { return __le__(other); }

		bool operator== (const T& other) const { return __eq__(other); }

		bool operator!= (const T& other) const { return __ueq__(other); }
	};
}
