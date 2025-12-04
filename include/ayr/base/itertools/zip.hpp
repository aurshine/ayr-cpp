#ifndef AYR_BASE_ITERTOOLS_ZIP_HPP
#define AYR_BASE_ITERTOOLS_ZIP_HPP

#include <tuple>

#include "IteratorInfo.hpp"

namespace ayr
{
	template<std::input_or_output_iterator ...Its>
	class ZipIterator : public IteratorInfo<ZipIterator<Its...>, NonContainer, std::input_iterator_tag, std::tuple<typename std::iterator_traits<Its>::reference...>>
	{
		using self = ZipIterator<Its...>;

		using ItInfo = IteratorInfo<self, NonContainer, std::input_iterator_tag, std::tuple<typename std::iterator_traits<Its>::reference...>>;

		std::tuple<Its...> its_;
	public:
		constexpr ZipIterator() : its_() {}

		constexpr ZipIterator(Its... its) : its_(std::move(its)...) {}

		constexpr ZipIterator(const self& other) : its_(other.its_) {}

		constexpr self& operator= (const self& other) { its_ = other.its_; return *this; }

		constexpr ItInfo::value_type operator*() const
		{
			return std::apply([](auto&&... its) { return typename ItInfo::value_type(*its...); }, its_);
		}

		constexpr self& operator++()
		{
			std::apply([](auto&&... its) { ((++its), ...); }, its_);
			return *this;
		}

		constexpr self operator++(int)
		{
			self tmp(*this);
			++(*this);
			return tmp;
		}

		constexpr bool operator==(const self& other) const
		{
			return ([&]<std::size_t... I>(std::index_sequence<I...>) {
				return ((std::get<I>(this->its_) == std::get<I>(other.its_)) || ...);
			})(std::index_sequence_for<Its...>{});
		}
	};

	template<std::input_or_output_iterator... Its>
	constexpr def zip_it(Its... its) -> ZipIterator<Its...>
	{
		return ZipIterator<Its...>(its...);
	}

	// 使用for (auto [a, b, c] : zip(a_range, b_range, c_range)) {... }
	template<Iteratable ...Itables>
	constexpr def zip(Itables&&... itables)
	{
		return std::ranges::subrange(
			zip_it(itables.begin()...),
			zip_it(itables.end()...)
		);
	}
}
#endif // AYR_BASE_ITERTOOLS_ZIP_HPP