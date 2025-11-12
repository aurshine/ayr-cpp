#ifndef AYR_BASE_ITERTOOLS_ENUMERATE_HPP
#define AYR_BASE_ITERTOOLS_ENUMERATE_HPP

#include "IteratorInfo.hpp"

namespace ayr
{
	template<std::input_or_output_iterator It>
	class EnumerateIterator : public IteratorInfo<EnumerateIterator<It>, NonContainer, std::input_iterator_tag, std::tuple<c_size, typename std::iterator_traits<It>::reference>>
	{
		using self = EnumerateIterator<It>;

		using ItInfo = IteratorInfo<self, NonContainer, std::input_iterator_tag, std::tuple<c_size, typename std::iterator_traits<It>::reference>>;

		c_size current_;

		It it_;
	public:
		constexpr EnumerateIterator() : current_(0), it_() {}

		constexpr EnumerateIterator(c_size current, It it) : current_(current), it_(std::move(it)) {}

		constexpr EnumerateIterator(const self& other) : current_(other.current_), it_(other.it_) {}

		constexpr self& operator= (const self& other) { current_ = other.current_; it_ = other.it_; return *this; }

		constexpr ItInfo::value_type operator*() const { return typename ItInfo::value_type(current_, *it_); }

		constexpr self& operator++() { ++current_; ++it_; return *this; }

		constexpr self operator++(int) { self tmp(*this); ++(*this); return tmp; }

		constexpr bool __equals__(const self& other) const { return it_ == other.it_; }
	};

	// 使用for (auto [i, elem] : enumerate(elems))
	template<Iteratable Obj>
	constexpr def enumerate(Obj&& elems)
	{
		using It = EnumerateIterator<std::ranges::iterator_t<Obj>>;

		return std::ranges::subrange(It(0, elems.begin()), It(-1, elems.end()));
	}
}
#endif // AYR_BASE_ITERTOOLS_ENUMERATE_HPP