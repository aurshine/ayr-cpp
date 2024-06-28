#pragma once
#include <type_traits>

#include <law/printer.hpp>
#include <law/DynArray.hpp>


namespace ayr
{
	// 自动管理内存创建节点类型
	template<typename T>
	class Creator : Object
	{
	public:
		template<typename... Args>
		T* create(Args&& ... args)
		{
			created_values_.append(T(std::forward<Args>(args)...));

			return &created_values_[-1];
		}

	private:
		DynArray<T> created_values_;
	};
}