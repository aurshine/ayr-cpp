#pragma once
#include "printer.hpp"
#include "object.hpp"

namespace ayr
{
	template<typename T>
	class Add : public Object
	{
	public:
		Add() {}
		~Add() {}

	public:
		// 对外接口，做加法运算并返回结果
		virtual T __add__(const T& other) = 0;

		// 对外接口，与自己做加法运算
		virtual void __iadd__(const T& other) = 0;

		T operator+ (const T& other)
		{
			return __add__(other);
		}

		T& operator+= (const T& other)
		{
			return __iadd__(other);
		}
	};
}
