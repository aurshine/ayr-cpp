#ifndef AYR_BASE_REFCOUNT_HPP
#define AYR_BASE_REFCOUNT_HPP

#include <atomic>
#include <memory>
#include "Object.hpp"

namespace ayr
{
	template<typename Derived>
	class RefCount : public Object<Derived>
	{
	public:
		using self = RefCount<Derived>;

		using super = Object<Derived>;

		RefCount() : count_(1) {}

		~RefCount() { subref(); }

		// 增加引用计数
		void addref() { count_.fetch_add(1); }

		// 减少引用计数
		void subref(std::uint32_t sub = 1)
		{
			if (count_.fetch_sub(sub) == sub)
				super::derived().__zeroref__();
		}

		// 引用计数为0后的操作
		void __zeroref__() { super::derived().__zeroref__(); }

	private:
		std::atomic_uint32_t count_;
	};
}
#endif