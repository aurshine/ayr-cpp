#ifndef AYR_ARRAY_HPP
#define AYR_ARRAY_HPP


#include "base/Array.hpp"
#include "base/ayr_concepts.hpp"


namespace ayr
{
	class Range : public Object<Range>
	{
	public:
		Range(c_size start, c_size end, c_size step = 1) : _start(start), _end(end), _step(step) {}

		Range(c_size end) : Range(0, end, 1) {}

		c_size size() const { return (_end - _start + _step - 1) / _step; }

		class RangeIterator : public IteratorInfo<RangeIterator, Range, std::bidirectional_iterator_tag, c_size>
		{
			using self = RangeIterator;

			using ItInfo = IteratorInfo<RangeIterator, Range, std::bidirectional_iterator_tag, c_size>;
		public:
			RangeIterator() : current_(0), step_(0) {}

			RangeIterator(c_size current, c_size step) : current_(current), step_(step) {}

			RangeIterator(const self& other) : current_(other.current_), step_(other.step_) {}

			self& operator= (const self& other)
			{
				current_ = other.current_;
				step_ = other.step_;
				return *this;
			}

			ItInfo::const_reference operator*() const { return current_; }

			ItInfo::const_pointer operator->() const { return &current_; }

			self& operator++() { current_ += step_; return *this; }

			self operator++(int) { self tmp(*this); ++(*this); return tmp; }

			self& operator--() { current_ -= step_; return *this; }

			self operator--(int) { self tmp(*this); --(*this); return tmp; }

			bool __equals__(const self& other) const { return current_ == other.current_ && step_ == other.step_; }
		private:
			c_size current_, step_;
		};

		RangeIterator begin() const { return RangeIterator(_start, _step); }

		RangeIterator end() const { return RangeIterator(_start + (_end - _start + _step - 1) / _step * _step, _step); }
	private:
		c_size _start, _end, _step;
	};

	template<Iteratable T, typename Init>
	def sum(T&& obj, Init init = Init()) -> Init
	{
		for (auto&& elem : obj)
			init += elem;

		return init;
	}
}

#endif