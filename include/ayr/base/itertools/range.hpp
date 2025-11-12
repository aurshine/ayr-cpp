#ifndef AYR_BASE_ITERTOOLS_RANGE_HPP
#define AYR_BASE_ITERTOOLS_RANGE_HPP

#include "IteratorInfo.hpp"

namespace ayr
{
	class RangeIterator : public IteratorInfo<RangeIterator, NonContainer, std::bidirectional_iterator_tag, c_size>
	{
		using self = RangeIterator;

		using ItInfo = IteratorInfo<RangeIterator, NonContainer, std::bidirectional_iterator_tag, c_size>;
	public:
		constexpr RangeIterator() : current_(0), step_(0) {}

		constexpr RangeIterator(c_size current, c_size step) : current_(current), step_(step) {}

		constexpr RangeIterator(const self& other) : current_(other.current_), step_(other.step_) {}

		constexpr self& operator= (const self& other)
		{
			current_ = other.current_;
			step_ = other.step_;
			return *this;
		}

		constexpr ItInfo::const_reference operator*() const { return current_; }

		constexpr ItInfo::const_pointer operator->() const { return &current_; }

		constexpr self& operator++() { current_ += step_; return *this; }

		constexpr self operator++(int) { self tmp(*this); ++(*this); return tmp; }

		constexpr self& operator--() { current_ -= step_; return *this; }

		constexpr self operator--(int) { self tmp(*this); --(*this); return tmp; }

		constexpr bool __equals__(const self& other) const { return current_ == other.current_ && step_ == other.step_; }
	private:
		c_size current_, step_;
	};

	constexpr def range(c_size start, c_size end, c_size step = 1)
	{
		end = start + (end - start + step - 1) / step * step;
		if (step == 0)
			ValueError("range step cannot be zero");
		if (start > end && step > 0)
			ValueError("range start > end with step > 0");
		if (start < end && step < 0)
			ValueError("range start < end with step < 0");
		return std::ranges::subrange(RangeIterator(start, step), RangeIterator(end, step));
	}

	constexpr def range(c_size end) { return range(0, end, 1); }
}
#endif // AYR_BASE_ITERTOOLS_RANGE_HPP