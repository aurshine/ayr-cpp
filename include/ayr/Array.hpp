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

	template<std::input_or_output_iterator ...Its>
	class ZipIterator : public IteratorInfo<ZipIterator<Its...>, NonContainer, std::forward_iterator_tag, std::tuple<typename std::iterator_traits<Its>::value_type&...>>
	{
	public:
		using self = ZipIterator<Its...>;

		using ItInfo = IteratorInfo<self, NonContainer, std::forward_iterator_tag, std::tuple<typename std::iterator_traits<Its>::value_type&...>>;

		ZipIterator() : its_() {}

		ZipIterator(Its... its) : its_(its...) {}

		ZipIterator(const self& other) : its_(other.its_) {}

		self& operator= (const self& other)
		{
			its_ = other.its_;
			return *this;
		}

		ItInfo::value_type operator*() const
		{
			return std::apply([](auto&&... its) { return typename ItInfo::value_type(*its...); }, its_);
		}

		self& operator++()
		{
			std::apply([](auto&&... its) { ((++its), ...); }, its_);
			return *this;
		}

		self operator++(int)
		{
			self tmp(*this);
			++(*this);
			return tmp;
		}

		bool __equals__(const self& other) const
		{
			return its_ == other.its_;
		}

	private:
		std::tuple<Its...> its_;
	};

	template<std::input_or_output_iterator... Its>
	def zip_it(Its... its) -> ZipIterator<Its...>
	{
		return ZipIterator<Its...>(its...);
	}

	template<Iteratable ...Itables>
	def zip(Itables&&... itables)
	{
		return std::ranges::subrange(
			zip_it(itables.begin()...),
			zip_it(itables.end()...)
		);
	}

	template<typename It>
	class EnumerateIterator : public IteratorInfo<EnumerateIterator<It>, NonContainer, std::forward_iterator_tag, std::tuple<c_size&, typename std::iterator_traits<It>::value_type&>>
	{
		using self = EnumerateIterator<It>;

		using ItInfo = IteratorInfo<self, NonContainer, std::forward_iterator_tag, std::tuple<c_size, typename std::iterator_traits<It>::value_type&>>;

		c_size current_;

		It it_;
	public:
		EnumerateIterator() : current_(0), it_() {}

		EnumerateIterator(c_size current, It it) : current_(current), it_(it) {}

		EnumerateIterator(const self& other) : current_(other.current_), it_(other.it_) {}

		self& operator= (const self& other) { current_ = other.current_; it_ = other.it_; return *this; }

		ItInfo::value_type operator*() const { return typename ItInfo::value_type(current_, *it_); }

		self& operator++() { ++current_; ++it_; return *this; }

		self operator++(int) { self tmp(*this); ++(*this); return tmp; }

		bool __equals__(const self& other) const { return it_ == other.it_; }
	};

	template<Iteratable Obj>
	def enumerate(Obj&& obj)
	{
		using It = EnumerateIterator<decltype(obj.begin())>;

		return std::ranges::subrange(It(0, obj.begin()), It(-1, obj.end()));
	}

	template<Iteratable T, typename Init>
	def sum(T&& obj, Init init = Init()) -> Init
	{
		for (auto&& elem : obj)
			init += elem;

		return init;
	}
}

#endif