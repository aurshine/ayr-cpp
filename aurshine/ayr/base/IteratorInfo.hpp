#ifndef AYR_BASE_ITERATORINFO_HPP
#define AYR_BASE_ITERATORINFO_HPP

#include <xutility>

#include "Object.hpp"

namespace ayr
{
	template<
		typename Container,
		typename Category,
		typename T,
		typename Distance = std::ptrdiff_t,
		typename Pointer = T*,
		typename Reference = T&>
	struct IteratorInfo :
		public Object<IteratorInfo<Container, Category, T, Distance, Pointer, Reference>>
	{
	public:
		using container_type = Container;

		using iterator_category = Category;

		using value_type = T;

		using difference_type = Distance;

		using pointer = Pointer;

		using reference = Reference;

		using const_pointer = const T*;

		using const_reference = const T&;
	};

}
#endif // AYR_BASE_ITERATORINFO_HPP