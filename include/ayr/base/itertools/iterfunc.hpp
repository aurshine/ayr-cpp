#ifndef AYR_BASE_ITERTOOLS_ITERFUNC_HPP
#define AYR_BASE_ITERTOOLS_ITERFUNC_HPP

#include "IteratorInfo.hpp"

#undef max
#undef min

namespace ayr
{
	// 求和
	template<Iteratable Obj, typename Init = std::ranges::range_value_t<Obj>>
	def sum(Obj&& elems, Init init = Init()) -> Init
	{
		for (auto&& elem : elems)
			init += elem;

		return init;
	}

	// 最大值
	template<Iteratable Obj>
	def max(Obj&& elems)
	{
		auto it = elems.begin();
		auto end_it = elems.end();
		if (it == end_it)
			RAISE("RangeError", "max() arg is an empty sequence");

		auto max_elem = *it;
		while (++it != end_it)
			if (*it > max_elem)
				max_elem = *it;
		return max_elem;
	}

	// 最小值
	template<Iteratable Obj>
	def min(Obj&& elems)
	{
		auto it = elems.begin();
		auto end_it = elems.end();
		if (it == end_it)
			RAISE("RangeError", "min() arg is an empty sequence");

		auto min_elem = *it;
		while (++it != end_it)
			if (*it < min_elem)
				min_elem = *it;
		return min_elem;
	}

	// 所有元素都为真
	template<IteratableU<bool> Obj>
	def all(Obj&& elems) -> bool
	{
		for (bool elem : elems)
			if (!elem)
				return false;
		return true;
	}

	// 所有元素都满足条件f
	template<Iteratable Obj, typename F>
	def all(Obj&& elems, F&& f) -> bool
	{
		for (auto&& elem : elems)
			if (!f(elem))
				return false;
		return true;
	}

	// 至少有一个元素为真
	template<IteratableU<bool> Obj>
	def any(Obj&& elems) -> bool
	{
		for (bool elem : elems)
			if (elem)
				return true;
		return false;
	}

	// 至少有一个元素满足条件f
	template<Iteratable Obj, typename F>
	def any(Obj&& elems, F&& f) -> bool
	{
		for (auto&& elem : elems)
			if (f(elem))
				return true;
		return false;
	}
}

#endif // AYR_BASE_ITERTOOLS_ITERFUNC_HPP