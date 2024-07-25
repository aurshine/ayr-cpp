#pragma once
#include <law/detail/printer.hpp>
#include <law/detail/IndexIterator.hpp>


namespace ayr
{
	template<typename T>
	class Sequence: public IndexContainer<Sequence<T>, T>
	{
		using self = Sequence<T>;

		using super = IndexContainer<self, T>;
	public:
		using Value_t = T;

		virtual Value_t& __at__(c_size) { NotImplementedError("__at__ not implemented"); return None<Value_t>; }

		virtual const Value_t& __at__(c_size) const { NotImplementedError("__at__ const not implemented"); return None<Value_t>; }

		virtual c_size size() const { NotImplementedError("Sequence::size not implemented"); return 0; }

		self& __iter_container__() const { return const_cast<self&>(*this); }

		Value_t& operator[] (c_size index) { return __at__(neg_index(index, size())); }

		const Value_t& operator[] (c_size index) const { return __at__(neg_index(index, size())); }

		cmp_t __cmp__(const self& other) const
		{
			c_size m_size = size(), o_size = other.size();
			for (c_size i = 0; i < m_size && i < o_size; ++i)
				if (operator[](i) < other[i])
					return -1;
				else if (operator[](i) > other[i])
					return 1;
			return m_size - o_size;
		}

		CString __str__() const
		{
			std::stringstream ss;
			ss << "Sequence<" << dtype(Value_t) << ">(";
			for (auto&& value : *this)
				ss << printer(value) << ", ";
			ss << ")";
			return CString(ss.str());
		}

		bool contains(const Value_t& v) const { return find(v) != super::end(); }

		super::ConstIterator find(const Value_t& v) const
		{
			for (auto it = super::begin(); it != super::end(); ++it)
				if (*it == v)
					return it;
			return super::end();
		}
	};
}