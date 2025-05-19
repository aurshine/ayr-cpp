#ifndef AYR_CORO_GENERATOR_HPP
#define AYR_CORO_GENERATOR_HPP

#include "Promise.hpp"
#include "../base/IteratorInfo.hpp"

namespace ayr
{
	namespace coro
	{
		struct GenStatus
		{
			using status_type = uint8_t;

			// 当前状态是yield
			static constexpr status_type YIELD = 0;

			// 当前状态是return
			static constexpr status_type RETURN = 1;

			// 当前状态已经结束协程
			static constexpr status_type DONE = 2;
		};

		template<typename T>
		struct GenPromise : public PromiseImpl<T>
		{
			using self = GenPromise<T>;

			using super = PromiseImpl<T>;

			using co_type = std::coroutine_handle<self>;

			GenPromise() = default;

			std::suspend_never initial_suspend() const noexcept { return {}; }

			std::suspend_always yield_value(T value) noexcept
			{
				super::value_ = std::move(value);
				return {};
			}

			void return_value(T value) noexcept
			{
				super::value_ = std::move(value);
				has_return = true;
			}

			co_type get_return_object() { return co_type::from_promise(*this); }

			bool has_return = false;
		};

		template<typename T>
		class Generator : public Object<Generator<T>>
		{
			static_assert(std::is_default_constructible_v<T>, "Generator requires default constructible result type");

			using self = Generator<T>;
		public:
			using promise_type = GenPromise<T>;

			using co_type = promise_type::co_type;

			co_type coro_;

			Generator(co_type coro) : coro_(coro) {}

			Generator(self&& other) : coro_(std::move(other)) { other.coro_ = nullptr; }

			~Generator() { if (coro_) { coro_.destroy(); coro_ = nullptr; } }

			self& operator=(self&& other)
			{
				if (this == &other) return *this;
				ayr_destroy(this);
				return *ayr_construct(this, std::move(other));
			}

			T& result() noexcept { return coro_.promise().result(); }

			const T& result() const noexcept { return coro_.promise().result(); }

			struct GeneratorIterator : public IteratorInfo<GeneratorIterator, Generator<T>, std::forward_iterator_tag, T>
			{
				using self = GeneratorIterator;

				using ItInfo = IteratorInfo<GeneratorIterator, Generator<T>, std::forward_iterator_tag, T>;

				GeneratorIterator(ItInfo::container_type* gen, GenStatus::status_type status = GenStatus::YIELD) : gen_(gen), status_(status) {}

				GeneratorIterator(self&& other) : gen_(other.gen_) { other.gen_ = nullptr; }

				self& operator++()
				{
					switch (status_)
					{
					case GenStatus::YIELD:
						yield_next();
						break;
					case GenStatus::RETURN:
						return_next();
						break;
					case GenStatus::DONE:
						RuntimeError("Generator already finished");
					}
					return *this;
				}

				ItInfo::reference operator*() const { return gen_->result(); }

				ItInfo::pointer operator->() const { return &gen_->result(); }

				bool __equals__(const self& other) const { return gen_ == other.gen_ && status_ == other.status_; }
			private:
				// 当前状态是yield, 进入下一个状态
				void yield_next()
				{
					gen_->coro_.resume();
					if (gen_->coro_.done())
						if (gen_->coro_.promise().has_return)
							status_ = GenStatus::RETURN;
						else
							status_ = GenStatus::DONE;
				}

				// 当前状态是return, 进入下一个状态
				void return_next() { status_ = GenStatus::DONE; }

				ItInfo::container_type* gen_;

				GenStatus::status_type status_;
			};

			GeneratorIterator begin() { return GeneratorIterator(this); }

			GeneratorIterator end() { return GeneratorIterator(this, GenStatus::DONE); }
		};
	}
}
#endif