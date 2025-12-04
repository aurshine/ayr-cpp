#ifndef AYR_BASE_ITERTOOLS_ITERATORINFO_HPP
#define AYR_BASE_ITERTOOLS_ITERATORINFO_HPP

#include <ranges>

#include "../raise_error.hpp"

namespace ayr
{
	struct NonContainer {};

	template<
		typename Iterator,
		typename Container,
		typename Category,
		typename T,
		typename Distance = std::ptrdiff_t,
		typename Pointer = T*,
		typename Reference = T&>
	struct IteratorInfo
	{
	public:
		using iterator_type = Iterator;

		using container_type = Container;

		using iterator_category = Category;

		using value_type = T;

		using difference_type = Distance;

		using pointer = Pointer;

		using reference = Reference;

		using rvalue_reference = std::remove_reference_t<T>&&;

		using const_pointer = const std::remove_reference_t<T>*;

		using const_reference = const std::remove_reference_t<T>&;
	};

}
#endif // AYR_BASE_ITERTOOLS_ITERATORINFO_HPP