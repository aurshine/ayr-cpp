#ifndef AYR_AYR_MEMORY_HPP
#define AYR_AYR_MEMORY_HPP

#include "base/ayr_memory.hpp"
#include "base/ayr_traits.hpp"
#include "DynArray.hpp"


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

	template<typename T>
	struct Ayrocator : Object<Ayrocator<T>>
	{
		// 分配并构造对象
		template<typename... Args>
		T* create(Args&&... args)
		{
			return *ayr_construct(ayr_alloc<T>(1), std::forward<Args>(args)...);
		}

		// 销毁对象
		void destroy(T* ptr)
		{
			ayr_delloc(ptr);
			ayr_destroy(ptr);
		}
	};
}

#endif