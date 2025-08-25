#ifndef AYR_BASE_VIEW_HPP
#define AYR_BASE_VIEW_HPP

#include "raise_error.hpp"

namespace ayr
{
	/*
	* @brief 视图类型，内部保存各种类型的引用
	* 
	* @detail View对象可以直接使用==和!=运算符进行比较，内部会自动转化为相应的类型进行比较。
	* 
	* @note 构造的对象必须是左值引用或View类型对象
	*/
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

		template<typename T>
		T& get() { return *static_cast<T*>(view_ptr_); }

		template<typename T>
		const T& get() const { return *static_cast<const T*>(view_ptr_); }

		template<typename T>
		operator T& () { return get<T>(); }

		template<typename T>
		operator const T& () const { return get<T>(); }

		template<typename T>
		bool __equals__(const T& other) const { return view_ptr_ == &other || get<T>() == other; }
	};

	// T类型的只读视图
	template<typename T>
	class ViewOF
	{
		static_assert(std::is_object_v<T> || std::is_function_v<T>, "T must be an object or function type");

		View view_;

		using Value_t = std::remove_const_t<T>;

		using self = ViewOF<T>;

		friend ViewOF<const Value_t>;

		friend ViewOF<Value_t>;
	public:
		ViewOF(const Value_t& obj) : view_(obj) {}

		ViewOF(Value_t&& obj) = delete;

		ViewOF(const ViewOF<Value_t>& other) : view_(other.view_) {}

		ViewOF(const ViewOF<const Value_t>& other) : view_(other.view_) {}

		self& operator=(const Value_t& obj)
		{
			view_ = obj;
			return *this;
		}

		self& operator=(Value_t&& obj) = delete;

		self& operator=(const ViewOF<Value_t>& other)
		{
			view_ = other.view_;
			return *this;
		}

		self& operator=(const ViewOF<const Value_t>& other)
		{
			view_ = other.view_;
			return *this;
		}

		const Value_t& get() const { return view_.get<const Value_t>(); }

		const Value_t* operator->() const { return static_cast<const Value_t*>(view_.view_ptr_); }

		template<typename... Args>
		auto operator()(Args&&... args) const
		{
			return std::invoke(get(), std::forward<Args>(args)...);
		}

		operator const Value_t& () const { return get(); }

		CString __str__() const { return cstr(get()); }

		void __repr__(Buffer& buffer) const { buffer << get(); }

		bool __equals__(const Value_t& other) const { return view_.__equals__(other); }

		void __swap__(ViewOF<Value_t>& other) { swap(view_, other.view_); }

		void __swap__(ViewOF<const Value_t>& other) { swap(view_, other.view_); }
	};

	template<typename T>
	def view_of(T& obj) { return ViewOF<const std::decay_t<T>>(obj); }

	template<typename T>
	def view_of(ViewOF<T> obj) { return obj; }
}
#endif