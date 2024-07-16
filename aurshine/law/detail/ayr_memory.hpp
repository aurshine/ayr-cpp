#pragma once
#include <memory>

namespace ayr
{
	template<typename T>
	inline T* allocate(size_t size) { return reinterpret_cast<T*>(::operator new(sizeof(T) * size)); }

	// 释放allocate分配的内存
	template<typename T>
	inline void deallocate(T* ptr) { ::operator delete[](ptr); }


	template<typename T, typename... Args>
	inline void construct_at(T* ptr, Args&&... args) { ::new(ptr) T(std::forward<Args>(args)...); }


	template<typename T>
	inline void destroy_at(T* ptr) { ptr->~T(); }
}