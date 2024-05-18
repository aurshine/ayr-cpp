#pragma once
#include "object.hpp"
#include "iterator/IteratorExecutor.hpp"

namespace ayr
{
	template<class T>
	class Container : public Object
	{
	public:
		virtual ~Container() { this->release(); }

	public:
		virtual bool empty() const { return size() == 0; }

		virtual void clear() {}

		// 清空容器并释放资源
		virtual release() {}

		virtual size_t size() const = 0;

		virtual bool contains(const T& item) const = 0;

		virtual void insert(const T& item) = 0;

		virtual void remove(const T& item) = 0;

		virtual IteratorExecutor<T> __iter__() = 0;

		IteratorExecutor<T> begin() { return __iter__(); }

		IteratorExecutor<T> end() { return IteratorExecutor<T>(nullptr); };

		const IteratorExecutor<T> cbegin() const { return __iter__(); };

		const IteratorExecutor<T> cend() const { return IteratorExecutor<T>(nullptr); };

		virtual IteratorExecutor<T> rbegin() = 0;

		virtual IteratorExecutor<T> rend() { return IteratorExecutor<T>(nullptr); };
	};
}