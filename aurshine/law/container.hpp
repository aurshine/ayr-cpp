#pragma once
#include "implemented.hpp"
#include "iterator/IteratorExecutor.hpp"

namespace ayr
{
	// 描述容器尺寸的类型
	using c_size = int64_t;

	template<class T>
	class Container : public Object
	{
	public:
		Container() = default;

		virtual ~Container() = default;

	public:
		virtual c_size size() const no_implement_v(0)

		virtual bool contains(const T& item) const no_implement_v(false)

		virtual IteratorExecutor<T> __iter__() no_implement_v(IteratorExecutor<T>(nullptr));

		IteratorExecutor<T> begin() { return __iter__(); }

		IteratorExecutor<T> end() { return IteratorExecutor<T>(nullptr); };

		const IteratorExecutor<T> cbegin() const { return __iter__(); };

		const IteratorExecutor<T> cend() const { return IteratorExecutor<T>(nullptr); };

		virtual IteratorExecutor<T> rbegin() no_implement_v(IteratorExecutor<T>(nullptr));

		virtual IteratorExecutor<T> rend() { return IteratorExecutor<T>(nullptr); };
	};
}