#pragma
#include "printer.hpp"

namespace ayr
{
	template<class T>
	class Container : public Object
	{
	public:
		virtual size_t size() const
		{
			error_assert(false, "正在使用Container的默认 size 函数");
			return 0;
		}

		template<class Key>
		T& operator[](const Key& item)
		{
			error_assert(false, "正在使用Container的默认 operator[] 函数");
			return T();
		}

		template<class Key>
		T operator[](const Key& item) const
		{
			error_assert(false, "正在使用Container的默认 operator[] 函数");
			return T();
		}

		virtual bool contains(const T& item)
		{
			warn_assert(false, "正在使用Container的默认 contains 函数");
			return false;
		}

		template<class Ty>
		void add(Ty&& item)
		{
			warn_assert(false, "正在使用Container的默认 add 函数");
		}

		template<class Ty>
		void add(const Ty& item)
		{
			warn_assert(false, "正在使用Container的默认 add 函数");
		}

		virtual void remove(const T& item)
		{
			warn_assert(false, "正在使用Container的默认 remove 函数");
		}
	};
}