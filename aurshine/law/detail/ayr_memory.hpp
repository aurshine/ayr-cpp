#pragma once
#include <memory>


namespace ayr
{
	// 分配连续size个T的内存, 不会调用构造函数
#define ayr_alloc(T, size) reinterpret_cast<T*>(::operator new(sizeof(T) * (size)))

// 在ptr上调用构造函数
#define ayr_construct(T, ptr, ...) ::new(ptr) T(__VA_ARGS__)

// 释放allocate分配的内存
#define ayr_delloc(ptr) ::operator delete[](ptr)

// 在ptr处调用T的析构函数, 释放内存
#define ayr_destroy(ptr) (ptr)->~T()
}