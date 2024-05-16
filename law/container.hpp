#pragma once
#include "object.hpp"
#include "iterator.hpp"

namespace ayr
{
	template<class T>
	class Container : public Object
	{
	public:
		virtual ~Container()
		{
			this->release();
		}

	public:
		virtual size_t size() const = 0;

		virtual bool empty() const
		{
			return size() == 0;
		}

		virtual bool contains(const T& item) const = 0;

		virtual void insert(const T& item) = 0;

		virtual void remove(const T& item) = 0;

		virtual void clear() {}

		// 清空容器并释放资源
		virtual release() {}

		virtual Iterator<T>* begin() = 0;

		virtual Iterator<T>* end() = 0;

		virtual const Iterator<T>* cbegin() const = 0;

		virtual const Iterator<T>* cend() const = 0;

		virtual Iterator<T>* rbegin() = 0;

		virtual Iterator<T>* rend() = 0;
	};
}