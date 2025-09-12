#ifndef AYR_OPTIONAL_HPP
#define AYR_OPTIONAL_HPP

#include "base/raise_error.hpp"

namespace ayr
{
	template<typename T>
	class Optional : public Object<T>
	{
		using self = Optional<T>;

		std::aligned_storage_t<sizeof(T), alignof(T)> storage_;

		bool has_value_;
	public:
		static_assert(!std::is_reference_v<T>, "Optional does not support reference types");

		Optional() : has_value_(false) {}

		Optional(const T& value) : has_value_(false) { emplace(value); }

		Optional(T&& value) : has_value_(false) { emplace(std::move(value)); }

		Optional(const self& other) : has_value_(false)
		{
			if (other.has_value())
				emplace(*other.get_ptr());
		}

		Optional(self&& other) : has_value_(false)
		{
			if (other.has_value())
			{
				emplace(std::move(*other.get_ptr()));
				other.reset();
			}
		}

		~Optional() { reset(); }

		self& operator=(const self& other)
		{
			if (this == &other) return *this;
			if (other.has_value())
				emplace(*other.get_ptr());
			else
				reset();
			return *this;
		}

		self& operator=(self&& other)
		{
			if (this == &other) return *this;
			if (other.has_value())
			{
				emplace(std::move(*other.get_ptr()));
				other.reset();
			}
			else
				reset();
			return *this;
		}

		self& operator=(const T& value) { emplace(value); return *this; }

		self& operator=(T&& value) { emplace(std::move(value)); return *this; }

		operator bool() const { return has_value_; }

		T& operator*() { return value(); }

		const T& operator*() const { return value(); }

		T* operator->() { return &value(); }

		const T* operator->() const { return &value(); }

		bool has_value() const { return has_value_; }

		void reset()
		{
			if (has_value())
			{
				ayr_destroy(get_ptr());
				has_value_ = false;
			}
		}

		template<typename... Args>
		T& emplace(Args&&... args)
		{
			T* ptr = get_ptr();
			if (has_value()) ayr_destroy(ptr);
			ayr_construct(ptr, std::forward<Args>(args)...);
			has_value_ = true;

			return *ptr;
		}

		T& value()
		{
			if (has_value()) return *get_ptr();
			RuntimeError("Optional does not have a value");
		}

		const T& value() const
		{
			if (has_value()) return *get_ptr();
			RuntimeError("Optional does not have a value");
		}

		T& value_or(T& default_value)
		{
			if (has_value())
				return *get_ptr();
			else
				return default_value;
		}

		const T& value_or(const T& default_value) const
		{
			if (has_value())
				return *get_ptr();
			else
				return default_value;
		}

		// 返回值类型为func装饰的函数的返回值类型
		// 如果有值，返回调用func(T&)的结果，否则返回空Optional
		template<typename F>
		auto map(F&& func) -> Optional<decltype(func(std::declval<T&>()))>
		{
			using result_type = decltype(func(std::declval<T&>()));
			if (has_value())
				return Optional<result_type>(func(*get_ptr()));
			return {};
		}

		// 返回值类型为func装饰的函数的返回值类型
		// 如果有值，返回调用func(T&)的结果，否则返回空Optional
		template<typename F>
		auto map(F&& func) const -> Optional<decltype(func(std::declval<T&>()))>
		{
			using result_type = decltype(func(std::declval<T&>()));
			if (has_value())
				return Optional<result_type>(func(*get_ptr()));
			return {};
		}

		// func(T&)返回值必须是Optional<U>
		// 如果有值，返回调用func(T&)的结果，否则返回空Optional
		template<typename F>
		auto and_then(F&& func) -> decltype(func(std::declval<T&>()))
		{
			using result_type = decltype(func(std::declval<T&>()));
			static_assert(is_optional_v<result_type>, "func must return an Optional");
			if (has_value())
				return func(*get_ptr());
			return {};
		}

		// func(T&)返回值必须是Optional<U>
		// 如果有值，返回调用func(T&)的结果，否则返回空Optional
		template<typename F>
		auto and_then(F&& func) const -> decltype(func(std::declval<T&>()))
		{
			using result_type = decltype(func(std::declval<T&>()));
			static_assert(is_optional_v<result_type>, "func must return an Optional");
			if (has_value())
				return func(*get_ptr());
			return {};
		}
		template<typename F>
		auto transform(F&& func)
		{
			using result_type = decltype(func(std::declval<T&>()));
			if constexpr (is_optional_v<result_type>)
				return and_then(std::forward<F>(func));
			else
				return map(std::forward<F>(func));
		}

		template<typename F>
		auto transform(F&& func) const
		{
			using result_type = decltype(func(std::declval<T&>()));
			if constexpr (is_optional_v<result_type>)
				return and_then(std::forward<F>(func));
			else
				return map(std::forward<F>(func));
		}

		// 如果有值，返回值，否则返回调用func()的结果
		template<typename F>
		self or_else(F&& func) const
		{
			if (has_value())
				return *get_ptr();
			return func();
		}

		// 如果有值且func(T&)返回true，返回值，否则返回空Optional
		template<typename F>
		self filter(F&& func) const
		{
			if (has_value() && func(*get_ptr()))
				return *get_ptr();
			return {};
		}

		cmp_t __cmp__(const self& other) const
		{
			if (has_value() && other.has_value())
				if (*get_ptr() > *other.get_ptr())
					return 1;
				else if (*get_ptr() < *other.get_ptr())
					return -1;
				else
					return 0;

			return has_value() - other.has_value();
		}

		bool __equals__(const self& other) const
		{
			if (has_value() && other.has_value())
				return *get_ptr() == *other.get_ptr();
			return has_value() == other.has_value();
		}

		hash_t __hash__() const
		{
			if (has_value())
				return ayrhash(*get_ptr());
			else
				return 0;
		}

		void __repr__(Buffer& buffer) const
		{
			if (has_value())
				buffer << *get_ptr();
			else
				buffer << "None";
		}

		CString __str__() const
		{
			if (has_value())
				return cstr(*get_ptr());
			return "None";
		}

		template<typename U>
		struct is_optional : std::false_type {};

		template<typename U>
		struct is_optional<Optional<U>> : std::true_type {};

		template<typename U>
		constexpr static bool is_optional_v = is_optional<T>::value;

	private:
		T* get_ptr() { return reinterpret_cast<T*>(&storage_); }

		const T* get_ptr() const { return reinterpret_cast<const T*>(&storage_); }
	};
}
#endif // AYR_OPTIONAL_HPP