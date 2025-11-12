#ifndef AYR_CORO_GENERATOR_HPP
#define AYR_CORO_GENERATOR_HPP

#include <coroutine>

#include "../base/itertools.hpp"
#include "../Optional.hpp"

namespace ayr
{
	namespace coro
	{
		struct _GenStatus
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
		class _GenPromise : public Object<_GenPromise<T>>
		{
			using self = _GenPromise<T>;
		public:
			static_assert(std::is_move_constructible_v<T>, "T must be move constructible");

			static_assert(!std::is_reference_v<T>, "T must not be a reference");

			using co_type = std::coroutine_handle<self>;

			std::suspend_never initial_suspend() const noexcept { return {}; }

			std::suspend_always final_suspend() const noexcept { return {}; }

			std::suspend_always yield_value(T value) noexcept
			{
				value_ = std::move(value);
				return {};
			}

			void return_value(T value) noexcept
			{
				value_ = std::move(value);
				has_return = true;
			}

			void unhandled_exception() { RuntimeError("Generator unhandled exception"); }

			co_type get_return_object() { return co_type::from_promise(*this); }

			T& result() noexcept { return *value_; }

			const T& result() const noexcept { return *value_; }

			Optional<T> value_;

			bool has_return = false;
		};

		template<typename T>
		class Generator : public Object<Generator<T>>
		{
			using self = Generator<T>;
		public:
			using promise_type = _GenPromise<T>;

			using co_type = promise_type::co_type;

			co_type coro_;

			Generator(co_type coro) : coro_(coro) {}

			Generator(self&& other) : coro_(other.coro_) { other.coro_ = nullptr; }

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

				GeneratorIterator(ItInfo::container_type* gen) : gen_(gen)
				{
					if (gen->coro_.done())
					{
						if (gen->coro_.promise().has_return)
							status_ = _GenStatus::RETURN;
						else
							status_ = _GenStatus::DONE;
					}
					else
						status_ = _GenStatus::YIELD;
				}

				GeneratorIterator(ItInfo::container_type* gen, _GenStatus::status_type status) : gen_(gen), status_(status) {}

				GeneratorIterator(self&& other) : gen_(other.gen_) { other.gen_ = nullptr; }

				self& operator++()
				{
					switch (status_)
					{
					case _GenStatus::YIELD:
						yield_next();
						break;
					case _GenStatus::RETURN:
						return_next();
						break;
					case _GenStatus::DONE:
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
							status_ = _GenStatus::RETURN;
						else
							status_ = _GenStatus::DONE;
				}

				// 当前状态是return, 进入下一个状态
				void return_next() { status_ = _GenStatus::DONE; }

				ItInfo::container_type* gen_;

				_GenStatus::status_type status_;
			};

			GeneratorIterator begin() { return GeneratorIterator(this); }

			GeneratorIterator end() { return GeneratorIterator(this, _GenStatus::DONE); }
		};
	}
}
#endif