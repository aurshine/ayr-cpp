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

		View(const self& other) : view_ptr_(other.view_ptr_) {}

		// View的构造对象只能是self或左值引用
		template<typename U>
			requires Or<issame<std::remove_reference_t<U>, self>, std::is_lvalue_reference_v<U>>
		View(U&& obj) : view_ptr_(nullptr)
		{
			if constexpr (issame<std::remove_reference_t<U>, self>)
				view_ptr_ = obj.view_ptr_;
			else if constexpr (std::is_lvalue_reference_v<U>)
				view_ptr_ = const_cast<std::decay_t<U>*>(std::addressof(obj));
		}

		self& operator=(const self& other)
		{
			view_ptr_ = other.view_ptr_;
			return *this;
		}

		template<typename U>
			requires Or<issame<std::remove_reference_t<U>, self>, std::is_lvalue_reference_v<U>>
		self& operator=(U&& obj)
		{
			return *ayr_construct(this, std::forward<U>(obj));
		}

		void* data() const { return view_ptr_; }

		template<typename T>
		T& get() { return *static_cast<T*>(view_ptr_); }

		template<typename T>
		const T& get() const { return *static_cast<const T*>(view_ptr_); }

		template<typename T>
		operator T& () { return get<T>(); }

		template<typename T>
		operator const T& () const { return get<T>(); }

		template<typename T>
		bool __equals__(const T& other) const { return data() == &other || get<T>() == other; }
	};

	// T类型的只读视图
	template<typename T>
	class ViewOF
	{
		View view_;

		using ConstValue_t = std::add_const_t<T>;
	public:
		ViewOF(ConstValue_t& obj): view_(obj) {}

		ViewOF(const ViewOF& other): view_(other.view_) {}

		ViewOF& operator=(ConstValue_t& obj)
		{
			view_ = obj;
			return *this;
		}

		ViewOF& operator=(const ViewOF& other)
		{
			view_ = other.view_;
			return *this;
		}

		ConstValue_t& get() const { return view_.get<ConstValue_t>(); }

		operator ConstValue_t& () const { return get(); }

		CString __str__() const { return cstr(get()); }

		bool __equals__(const ConstValue_t& other) const { return view_.__equals__(other); }

		void __swap__(ConstValue_t& other) { swap(view_, other); }
	};

	template<typename T>
	def view_of(T& obj) { return ViewOF<std::remove_const_t<T>>(obj); }

	template<typename T>
	def view_of(const ViewOF<T>& obj) { return ViewOF<T>(obj); }
}
#endif