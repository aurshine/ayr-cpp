#ifndef AYR_AYR_MEMORY_HPP
#define AYR_AYR_MEMORY_HPP

#include <ayr/detail/ayr_memory.hpp>
#include <ayr/detail/ayr_traits.hpp>
#include <ayr/DynArray.hpp>


namespace ayr
{
	// 自动管理内存创建节点类型
	template<typename T>
	class Creator : Object<Creator<T>>
	{
	public:
		Creator() = default;

		Creator(const Creator&) = delete;

		Creator& operator=(const Creator&) = delete;

		Creator(Creator&& other) noexcept : created_values(std::move(other.created_values)) {};

		Creator& operator=(Creator&& other) noexcept
		{
			created_values = std::move(other.created_values);
			return *this;
		}

		template<typename... Args>
		T* operator()(Args&& ... args)
		{
			return &created_values.append(T(std::forward<Args>(args)...));
		}
	private:
		DynArray<T> created_values;
	};
}

#endif