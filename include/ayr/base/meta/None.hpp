#ifndef AYR_BASE_META_NONE_HPP
#define AYR_BASE_META_NONE_HPP

#include "CString.hpp"

namespace ayr
{
	/*
	* @brief 表示一个空类型
	*/
	struct _None
	{
		constexpr _None() {}

		template<typename T>
		operator T& () const { return *static_cast<T*>(nullptr); }

		CString __str__() const { return "None"; }
	};

	// 空类型，不会应该使用，用于占位
	constexpr static _None None;
}
#endif // AYR_BASE_META_NONE_HPP