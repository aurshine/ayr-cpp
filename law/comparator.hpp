#pragma once
#include "printer.hpp"

namespace ayr
{
	template<class T>
	class Comparator : public Object
	{
	public:
		// a > b 等价于 ! a <= b
		virtual bool operator> (const T& other) const
		{
			warn_assert(false, "The default comparison function is being used");
			return !this->operator<= (other);
		}

		// a < b 等价于 ! a >= b
		virtual bool operator< (const T& other) const
		{
			warn_assert(false, "The default comparison function is being used");
			return !this->operator>= (other);
		}

		// a >= b 等价于 ! a < b
		virtual bool operator>= (const T& other) const
		{
			warn_assert(false, "The default comparison function is being used");
			return !this->operator< (other);
		}

		// a <= b 等价于 ! a > b
		virtual bool operator<= (const T& other) const
		{
			warn_assert(false, "The default comparison function is being used");
			return !this->operator> (other);
		}

		virtual bool operator== (const T& other) const
		{
			warn_assert(false, "The default comparison function is being used");
			return this == &other;
		}

		// a != b 等价于 ! a == b
		virtual bool operator!= (const T& other) const
		{
			warn_assert(false, "The default comparison function is being used");
			return !this->operator==(other);
		}
	};
}
