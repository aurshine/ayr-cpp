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
	class View
	{
		using self = View;

		void* view_ptr_;
	public:
		constexpr View() : view_ptr_(nullptr) {}

		constexpr View(const self& other) : view_ptr_(other.view_ptr_) {}

		// View的构造对象只能是self或左值引用
		template<typename U>
			requires Or<issame<std::remove_reference_t<U>, self>, std::is_lvalue_reference_v<U>>
		constexpr View(U&& obj) : view_ptr_(nullptr)
		{
			if constexpr (issame<std::remove_reference_t<U>, self>)
				view_ptr_ = obj.view_ptr_;
			else if constexpr (std::is_lvalue_reference_v<U>)
				view_ptr_ = const_cast<std::decay_t<U>*>(std::addressof(obj));
		}

		constexpr self& operator=(const self& other)
		{
			view_ptr_ = other.view_ptr_;
			return *this;
		}

		template<typename U>
			requires Or<issame<std::remove_reference_t<U>, self>, std::is_lvalue_reference_v<U>>
		constexpr self& operator=(U&& obj)
		{
			return *ayr_construct(this, std::forward<U>(obj));
		}

		template<typename T>
		constexpr T& get() { return *static_cast<T*>(view_ptr_); }

		template<typename T>
		constexpr const T& get() const { return *static_cast<const T*>(view_ptr_); }

		template<typename T>
		constexpr operator T& () { return get<T>(); }

		template<typename T>
		constexpr operator const T& () const { return get<T>(); }

		template<typename T>
		constexpr bool operator==(const T& other) const { return view_ptr_ == &other || get<T>() == other; }
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
		constexpr ViewOF(const Value_t& obj) : view_(obj) {}

		ViewOF(Value_t&& obj) = delete;

		constexpr ViewOF(const ViewOF<Value_t>& other) : view_(other.view_) {}

		constexpr ViewOF(const ViewOF<const Value_t>& other) : view_(other.view_) {}

		constexpr self& operator=(const Value_t& obj)
		{
			view_ = obj;
			return *this;
		}

		self& operator=(Value_t&& obj) = delete;

		constexpr self& operator=(const ViewOF<Value_t>& other)
		{
			view_ = other.view_;
			return *this;
		}

		constexpr self& operator=(const ViewOF<const Value_t>& other)
		{
			view_ = other.view_;
			return *this;
		}

		constexpr const Value_t& get() const { return view_.get<const Value_t>(); }

		constexpr const Value_t* operator->() const { return static_cast<const Value_t*>(view_.view_ptr_); }

		template<typename... Args>
		constexpr auto operator()(Args&&... args) const
		{
			return std::invoke(get(), std::forward<Args>(args)...);
		}

		constexpr operator const Value_t& () const { return get(); }

		constexpr std::strong_ordering operator<=>(const self& other) const { return get() <=> other.get(); }

		constexpr std::strong_ordering operator<=>(const Value_t& other) const { return get() <=> other; }

		constexpr bool operator==(const self& other) const { return get() == other.get(); }

		constexpr bool operator==(const Value_t& other) const { return get() == other; }

		CString __str__() const { return cstr(get()); }

		void __repr__(Buffer& buffer) const { buffer << get(); }
	};

	template<typename T>
	def view_of(T& obj) { return ViewOF<const std::decay_t<T>>(obj); }

	template<typename T>
	def view_of(ViewOF<T> obj) { return obj; }
}
#endif // AYR_BASE_VIEW_HPP