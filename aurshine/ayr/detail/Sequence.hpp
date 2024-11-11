#ifndef AYR_DETAIL_SEQUENCE_HPP
#define AYR_DETAIL_SEQUENCE_HPP

#include <ayr/detail/printer.hpp>
#include <ayr/detail/IndexIterator.hpp>


namespace ayr
{
	// 需要子类实现 size() 和 at()
	template<typename Derived, typename T>
	class Sequence : public Object<Sequence<Derived, T>>
	{
	public:
		using self = Sequence<Derived, T>;

		using super = Object<Sequence<Derived, T>>;
	public:
		using Value_t = T;

		using Iterator = IndexIterator<false, self, Value_t>;

		using ConstIterator = IndexIterator<true, self, Value_t>;

		Value_t& at(c_size index) { return static_cast<Derived*>(this)->at(index); }

		const Value_t& at(c_size index) const { return static_cast<const Derived*>(this)->at(index); }

		c_size size() const { return static_cast<const Derived*>(this)->size(); }

		cmp_t __cmp__(const Derived& other) const
		{
			auto m_it = begin(), m_end = end();
			auto o_it = other.begin(), o_end = other.end();
			while (m_it != m_end && o_it != o_end)
			{
				if (*m_it < *o_it) return -1;
				if (*m_it > *o_it) return 1;
				++m_it, ++o_it;
			}

			if (m_it != m_end) return 1;
			if (o_it != o_end) return -1;
			return 0;
		}

		bool __equals__(const Derived& other) const
		{
			if (size() != other.size())
				return false;

			return static_cast<const Derived*>(this)->__cmp__(other) == 0;
		}

		Iterator begin() { return Iterator(this, 0); }

		Iterator end() { return Iterator(this, size()); }

		ConstIterator begin() const { return ConstIterator(this, 0); }

		ConstIterator end() const { return ConstIterator(this, size()); }

		Value_t& operator[] (c_size index) { return at(neg_index(index, size())); }

		const Value_t& operator[] (c_size index) const { return at(neg_index(index, size())); }

		bool contains(const Value_t& v) const { return find_it(v) != end(); }

		c_size find(const Value_t& v) const
		{
			for (c_size i = 0, size_ = size(); i < size_; ++i)
				if (at(i) == v)
					return i;

			return -1;
		}

		Iterator find_it(const Value_t& v)
		{
			auto it = begin(), end_ = end();
			while (it != end_)
			{
				if (*it == v)
					return it;

				++it;
			}

			return end_;
		}

		ConstIterator find_it(const Value_t& v) const
		{
			auto it = begin(), end_ = end();
			while (it != end_)
			{
				if (*it == v)
					return it;

				++it;
			}

			return end_;
		}
	};
}

#endif