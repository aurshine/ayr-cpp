#ifndef AYR_BASE_VIEW_HPP
#define AYR_BASE_VIEW_HPP

#include "raise_error.hpp"

namespace ayr
{
	template<typename T>
	class View
	{
		static_assert(std::is_object_v<T>, "T must be an object type");

		T* view_ptr_;
	public:
		template<typename U>
			requires std::convertible_to<U, T>
		View(U& view_ref) : view_ptr_(std::addressof(static_cast<T&>(view_ref))) {}

		T& get() { return *view_ptr_; }

		const T& get() const { return *view_ptr_; }

		operator T& () { return *view_ptr_; }

		operator const T& () const { return *view_ptr_; }
	};
}
#endif