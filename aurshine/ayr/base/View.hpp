#ifndef AYR_BASE_VIEW_HPP
#define AYR_BASE_VIEW_HPP

#include "raise_error.hpp"

namespace ayr
{
	class View : public Object<View>
	{
		using self = View;

		void* view_ptr_;
	public:
		View() : view_ptr_(nullptr) {}

		template<typename U>
			requires Or<issame<std::remove_reference_t<U>, self>, std::is_lvalue_reference_v<U>>
		View(U&& obj) : view_ptr_(nullptr)
		{
			if constexpr (issame<std::remove_reference_t<U>, self>)
				view_ptr_ = obj.view_ptr_;
			else if constexpr (std::is_lvalue_reference_v<U>)
				view_ptr_ = const_cast<std::decay_t<U>*>(std::addressof(obj));
		}

		template<typename U>
			requires Or<issame<std::remove_reference_t<U>, self>, std::is_lvalue_reference_v<U>>
		self& operator=(U&& obj)
		{
			if constexpr (issame<std::remove_reference_t<U>, self>)
				view_ptr_ = obj.view_ptr_;
			else if constexpr (std::is_lvalue_reference_v<U>)
				view_ptr_ = const_cast<std::decay_t<U>*>(std::addressof(obj));
			return *this;
		}

		template<typename T>
		T& get() { return *static_cast<T*>(view_ptr_); }

		template<typename T>
		const T& get() const { return *static_cast<const T*>(view_ptr_); }

		template<typename T>
		operator T& () { return get<T>(); }

		template<typename T>
		operator const T& () const { return get<T>(); }
	};
}
#endif