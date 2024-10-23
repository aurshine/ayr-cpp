#ifndef AYR_LAW_DETAIL_AYR_MEMORY_HPP
#define AYR_LAW_DETAIL_AYR_MEMORY_HPP

#include <memory>

#include <ayr/detail/ayr.h>

namespace ayr
{
	// 分配连续size个T的内存, 不会调用构造函数
	template<typename T>
	def ayr_alloc(size_t size) -> T*
	{
		if (size == 0) return nullptr;
		return reinterpret_cast<T*>(::operator new(sizeof(T) * size));
	}

	// 在ptr上调用构造函数, 并返回ptr
	template<typename T, typename ... Args>
	def ayr_construct(T* ptr, Args&&... args) -> T*
	{
		::new(ptr) T(std::forward<Args>(args)...);
		return ptr;
	}

	// 调用ptr的析构函数,不会释放内存
	template<typename T>
	def ayr_destroy(T* ptr)
	{
		ptr->~T();
	}

	// 释放ptr指向的内存
	template<typename T>
	def ayr_delloc(T* ptr)
	{
		::operator delete(ptr);
	}
}
#endif 