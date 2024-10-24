#ifndef AYR_CORO_GENERATOR_HPP
#define AYR_CORO_GENERATOR_HPP

#include "Awaiter.hpp"

namespace ayr
{
	namespace coro
	{
		template<typename T>
		struct GeneratorIterator : public Object<GeneratorIterator<T>>
		{
			using self = GeneratorIterator<T>;

			using co_type = std::coroutine_handle<Promise<T>>;

			using iterator_category = std::forward_iterator_tag;

			using value_type = T;

			using difference_type = std::ptrdiff_t;

			using pointer = value_type*;

			using const_pointer = const value_type*;

			using reference = value_type&;

			using const_reference = const value_type&;

			GeneratorIterator(co_type coroutine) : coro_(coroutine) { ++(*this); }

			GeneratorIterator(const self& other) = delete;

			GeneratorIterator& operator=(const self& other) = delete;

			GeneratorIterator(self&& other) : coro_(other.coro_) { other.coro_ = nullptr; }

			GeneratorIterator& operator++()
			{
				if (coro_)
				{
					coro_.resume();
					if (coro_.done()) { coro_.destroy(); coro_ = nullptr; }
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


		template<typename T>
		class Generator : public Awaiter<T>
		{
			using self = Generator<T>;

			using super = Awaiter<T>;
		public:
			using co_type = super::co_type;

			Generator(co_type coro) : super(coro) {}

			Generator(Generator&& other) : super(std::move(other)) {};

			Generator(const Generator& other) = delete;

			Generator& operator=(const Generator& other) = delete;

			Generator& operator=(Generator&& other)
			{
				super::operator=(std::move(other));
				return *this;
			}

			GeneratorIterator<T> begin() { return GeneratorIterator<T>(super::coro_); }

			GeneratorIterator<T> end() { return GeneratorIterator<T>(nullptr); }
		};
	}
}
#endif