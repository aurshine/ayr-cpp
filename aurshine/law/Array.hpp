#pragma once
#include <law/detail/Array.hpp>

namespace ayr
{
	class Range : public Object
	{
	public:
		Range(c_size start, c_size end, c_size step = 1) : _start(start), _end(end), _step(step) {}

		Range(c_size end) : Range(0, end, 1) {}

		class RangeIterator
		{
		public:
			RangeIterator(c_size current, c_size step) : current_(current), step_(step) {}

			c_size operator*() const { return current_; }

			RangeIterator& operator++()
			{
				current_ += step_;

				return *this;
			}

			bool operator!=(const RangeIterator& other) const { return current_ != other.current_; }

			cmp_t __cmp__(const RangeIterator& other) const { return current_ - other.current_; }

		private:
			c_size current_, step_;
		};

		RangeIterator begin() const { return RangeIterator(_start, _step); }

		RangeIterator end() const { return RangeIterator(_start + (_end - _start + _step - 1) / _step * _step, _step); }
	private:
		c_size _start, _end, _step;
	};
}