#pragma once
#include <law/detail/Array.hpp>

namespace ayr
{
	class Range : public Object
	{
	public:
		constexpr Range(c_size start, c_size end, c_size step = 1) : _start(start), _end(end), _step(step) {}

		constexpr Range(c_size end) : Range(0, end, 1) {}

		class RangeIterator: public IteratorImpl<c_size>
		{
			using self = RangeIterator;
		public:
			constexpr RangeIterator(c_size current, c_size step) : current_(current), step_(step) {}

			virtual Value_t& operator*() { return current_; }

			virtual Value_t* operator->() { return &current_; }

			virtual const Value_t& operator*() const { return current_; }

			virtual const Value_t* operator->() const { return &current_; }

			virtual self& operator++() { current_ += step_; return *this; }

			virtual self& operator--() { current_ -= step_; return *this; }

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
	inline Array<T> make_array(c_size size, const F& func)
	{
		Array<T> arr(size);
		for (c_size i = 0; i < size; ++i)
			arr[i] = func(i);

		return arr;
	}
}