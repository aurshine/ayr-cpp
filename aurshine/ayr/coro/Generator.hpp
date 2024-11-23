#ifndef AYR_CORO_GENERATOR_HPP
#define AYR_CORO_GENERATOR_HPP

#include <ayr/base/printer.hpp>
#include <ayr/base/NoCopy.hpp>

namespace ayr
{
	namespace coro
	{
		template<typename T>
		class Generator : public Object<Generator<T>>, public NoCopy
		{
			using self = Generator<T>;
			static_assert(std::is_default_constructible_v<T>, "Generator requires default constructible result type");
		public:
			struct promise_type : public PromiseImpl<T>
			{
				using super = PromiseImpl<T>;

				using co_type = std::coroutine_handle<promise_type>;

				std::suspend_always yield_value(T value) noexcept
				{
					super::value_ = std::move(value);
					return {};
				}

				self get_return_object() { return self(co_type::from_promise(*this)); }
			};

			using co_type = promise_type::co_type;

			Generator(co_type coro) : coro_(coro) {}

			Generator(Generator&& other) : coro_(std::move(other)) { other.coro_ = nullptr; };

			Generator& operator=(Generator&& other)
			{
				if (this != &other)
				{
					if (coro_) coro_.destroy();
					coro_ = other.coro_;
					other.coro_ = nullptr;
				}
				return *this;
			}

			~Generator() { if (coro_) { coro_.destroy(); coro_ = nullptr; } }

			struct GeneratorIterator : public Object<GeneratorIterator>
			{
				using self = GeneratorIterator;

				using iterator_category = std::forward_iterator_tag;

				using value_type = T;

				using difference_type = std::ptrdiff_t;

				using pointer = value_type*;

				using const_pointer = const value_type*;

				using reference = value_type&;

				using const_reference = const value_type&;

				GeneratorIterator(co_type coroutine) : coro_(coroutine) {}

				GeneratorIterator(self&& other) : coro_(other.coro_) { other.coro_ = nullptr; }

				GeneratorIterator& operator++()
				{
					if (coro_)
					{
						coro_.resume();
						if (coro_.done()) coro_ = nullptr;
					}
					return *this;
				}

				reference operator*() { return coro_.promise().result(); }

				const_reference operator*() const { return coro_.promise().result(); }

				pointer operator->() { return &coro_.promise().result(); }

				const_pointer operator->() const { return &coro_.promise().result(); }

				bool __equals__(const self& other) const { return coro_.address() == other.coro_.address(); }
			private:
				co_type coro_;
			};

			GeneratorIterator begin() { return GeneratorIterator(coro_); }

			GeneratorIterator end() { return GeneratorIterator(nullptr); }

		private:
			co_type coro_;
		};
	}
}
#endif