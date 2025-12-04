#ifndef AYR_AIR_OPTIONAL_HPP
#define AYR_AIR_OPTIONAL_HPP

#include "../base.hpp"

namespace ayr
{
	template<typename T>
	class Optional
	{
		using self = Optional<T>;

		std::aligned_storage_t<sizeof(T), alignof(T)> storage_;

		bool has_value_;
	public:
		static_assert(!std::is_reference_v<T>, "Optional does not support reference types");

		constexpr Optional() : has_value_(false) {}

		constexpr Optional(const T& value) : has_value_(false) { emplace(value); }

		constexpr Optional(T&& value) : has_value_(false) { emplace(std::move(value)); }

		constexpr Optional(const self& other) : has_value_(false)
		{
			if (other.has_value())
				emplace(*other.get_ptr());
		}

		constexpr Optional(self&& other) : has_value_(false)
		{
			if (other.has_value())
			{
				emplace(std::move(*other.get_ptr()));
				other.reset();
			}
		}

		constexpr ~Optional() { reset(); }

		constexpr self& operator=(const self& other)
		{
			if (this == &other) return *this;
			if (other.has_value())
				emplace(*other.get_ptr());
			else
				reset();
			return *this;
		}

		constexpr self& operator=(self&& other)
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

		constexpr self& operator=(const T& value) { emplace(value); return *this; }

		constexpr self& operator=(T&& value) { emplace(std::move(value)); return *this; }

		constexpr operator bool() const { return has_value_; }

		constexpr T& operator*() { return value(); }

		constexpr const T& operator*() const { return value(); }

		constexpr T* operator->() { return &value(); }

		constexpr const T* operator->() const { return &value(); }

		constexpr bool has_value() const { return has_value_; }

		constexpr void reset()
		{
			if (has_value())
			{
				ayr_destroy(get_ptr());
				has_value_ = false;
			}
		}

		template<typename... Args>
		constexpr T& emplace(Args&&... args)
		{
			T* ptr = get_ptr();
			if (has_value()) ayr_destroy(ptr);
			ayr_construct(ptr, std::forward<Args>(args)...);
			has_value_ = true;

			return *ptr;
		}

		constexpr T& value()
		{
			if (has_value()) return *get_ptr();
			RuntimeError("Optional does not have a value");
			return None;
		}

		constexpr const T& value() const
		{
			if (has_value()) return *get_ptr();
			RuntimeError("Optional does not have a value");
			return None;
		}

		constexpr T& value_or(T& default_value)
		{
			if (has_value())
				return *get_ptr();
			else
				return default_value;
		}

		constexpr const T& value_or(const T& default_value) const
		{
			if (has_value())
				return *get_ptr();
			else
				return default_value;
		}

		// 返回值类型为func装饰的函数的返回值类型
		// 如果有值，返回调用func(T&)的结果，否则返回空Optional
		template<typename F>
		constexpr auto map(F&& func) -> Optional<decltype(func(std::declval<T&>()))>
		{
			using result_type = decltype(func(std::declval<T&>()));
			if (has_value())
				return Optional<result_type>(func(*get_ptr()));
			return {};
		}

		// 返回值类型为func装饰的函数的返回值类型
		// 如果有值，返回调用func(T&)的结果，否则返回空Optional
		template<typename F>
		constexpr auto map(F&& func) const -> Optional<decltype(func(std::declval<T&>()))>
		{
			using result_type = decltype(func(std::declval<T&>()));
			if (has_value())
				return Optional<result_type>(func(*get_ptr()));
			return {};
		}

		// func(T&)返回值必须是Optional<U>
		// 如果有值，返回调用func(T&)的结果，否则返回空Optional
		template<typename F>
		constexpr auto and_then(F&& func) -> decltype(func(std::declval<T&>()))
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
		constexpr auto and_then(F&& func) const -> decltype(func(std::declval<T&>()))
		{
			using result_type = decltype(func(std::declval<T&>()));
			static_assert(is_optional_v<result_type>, "func must return an Optional");
			if (has_value())
				return func(*get_ptr());
			return {};
		}
		template<typename F>
		constexpr auto transform(F&& func)
		{
			using result_type = decltype(func(std::declval<T&>()));
			if constexpr (is_optional_v<result_type>)
				return and_then(std::forward<F>(func));
			else
				return map(std::forward<F>(func));
		}

		template<typename F>
		constexpr auto transform(F&& func) const
		{
			using result_type = decltype(func(std::declval<T&>()));
			if constexpr (is_optional_v<result_type>)
				return and_then(std::forward<F>(func));
			else
				return map(std::forward<F>(func));
		}

		// 如果有值，返回值，否则返回调用func()的结果
		template<typename F>
		constexpr self or_else(F&& func) const
		{
			if (has_value())
				return *get_ptr();
			return func();
		}

		// 如果有值且func(T&)返回true，返回值，否则返回空Optional
		template<typename F>
		constexpr self filter(F&& func) const
		{
			if (has_value() && func(*get_ptr()))
				return *get_ptr();
			return {};
		}

		constexpr std::strong_ordering operator<=>(const self& other) const
		{
			if (has_value() && other.has_value())
				return *get_ptr() <=> *other.get_ptr();
			return has_value() <=> other.has_value();
		}

		constexpr bool operator==(const self& other) const
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
#endif // AYR_AIR_OPTIONAL_HPP