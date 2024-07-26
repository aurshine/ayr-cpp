#pragma once
#include <law/detail/Array.hpp>
#include <law/detail/ayr_concepts.hpp>


namespace ayr
{
	class Range : public Object
	{
	public:
		Range(c_size start, c_size end, c_size step = 1) : _start(start), _end(end), _step(step) {}

		Range(c_size end) : Range(0, end, 1) {}

		class RangeIterator: public IteratorImpl<c_size>
		{
			using self = RangeIterator;

			using Value_t = c_size;
		public:
			RangeIterator(c_size current, c_size step) : current_(current), step_(step) {}

			Value_t& operator*() { return current_; }

			Value_t* operator->() { return &current_; }

			const Value_t& operator*() const { return current_; }

			const Value_t* operator->() const { return &current_; }

			self& operator++() { current_ += step_; return *this; }

			self& operator--() { current_ -= step_; return *this; }

			cmp_t __cmp__(const RangeIterator& other) const { return current_ - other.current_; }
		private:
			c_size current_, step_;
		};

		RangeIterator begin() const { return RangeIterator(_start, _step); }

		RangeIterator end() const { return RangeIterator(_start + (_end - _start + _step - 1) / _step * _step, _step); }
	private:
		c_size _start, _end, _step;
	};


	template<typename T, typename F>
	constexpr inline Array<T> make_array(c_size size, const F& func)
	{
		Array<T> arr(size);
		for (c_size i = 0; i < size; ++i)
			arr[i] = func(i);

		return arr;
	}
}