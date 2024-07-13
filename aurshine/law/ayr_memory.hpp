#pragma once
#include <type_traits>
#include <memory>

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
		T* operator()(Args&& ... args)
		{
			return &created_values.append(T(std::forward<Args>(args)...));
		}
	private:
		DynArray<T> created_values;
	};
}