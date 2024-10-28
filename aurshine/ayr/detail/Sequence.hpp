#ifndef AYR_DETAIL_SEQUENCE_HPP
#define AYR_DETAIL_SEQUENCE_HPP

#include <ayr/detail/printer.hpp>
#include <ayr/detail/IndexIterator.hpp>


namespace ayr
{
	template<typename T>
	class Sequence : public Object<Sequence<T>>
	{
	public:
		using self = Sequence<T>;

		using super = Object<Sequence<T>>;
	public:
		using Value_t = T;

		using Iterator = IndexIterator<false, self, Value_t>;

		using ConstIterator = IndexIterator<true, self, Value_t>;

		virtual Value_t& at(c_size) = 0;

		virtual const Value_t& at(c_size) const = 0;

		virtual c_size size() const = 0;

		/*virtual cmp_t __cmp__(const self& other) const
		{
			auto m_it = begin(), m_end = end(), o_it = other.begin(), o_end = other.end();
			while (m_it != m_end && o_it != o_end)
			{
				if (*m_it < *o_it)
					return -1;
				else if (*m_it > *o_it)
					return 1;

				++m_it, ++o_it;
			}

			if (m_it != m_end)
				return 1;
			else if (o_it != o_end)
				return -1;
			else
				return 0;
		}

		virtual bool __equals__(const self& other) const
		{
			if (size() != other.size())
				return false;

			return __cmp__(other) == 0;
		}*/

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