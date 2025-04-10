#ifndef AYR_BASE_SEQUENCE_HPP
#define AYR_BASE_SEQUENCE_HPP

#include <functional>

#include "IndexIterator.hpp"


namespace ayr
{
	// 需要子类实现 size() 和 at()
	template<typename Derived, typename T>
	class Sequence : public Object<Derived>
	{
	public:
		using self = Sequence<Derived, T>;

		using super = Object<Derived>;
	public:
		using Value_t = T;

		using Iterator = IndexIterator<false, self, Value_t>;

		using ConstIterator = IndexIterator<true, self, Value_t>;

		using CheckTask = std::function<bool(const Value_t&)>;

		const Value_t& at(c_size index) const { NotImplementedError(std::format("{} Not implemented at(c_size)", dtype(Derived))); return None<c_size>; }

		c_size size() const { NotImplementedError(std::format("{} Not implemented size()", dtype(Derived))); return None<c_size>; }

		// 删除最后n个元素
		void pop_back(c_size n = 1) { NotImplementedError(std::format("{} Not implemented pop_back(c_size)", dtype(Derived))); }

		Value_t& front() { return *super::derived().begin(); }

		const Value_t& front() const { return *super::derived().begin(); }

		Value_t& back() { return *std::next(super::derived().begin(), super::derived().size() - 1); }

		const Value_t& back() const { return *std::next(super::derived().begin(), super::derived().size() - 1); }

		Value_t& operator[] (c_size index) { return super::derived().at(neg_index(index, super::derived().size())); }

		const Value_t& operator[] (c_size index) const { return super::derived().at(neg_index(index, super::derived().size())); }

		bool contains(const Value_t& v) const { return find_it(v) != super::derived().end(); }

		//  遍历每个元素, 并执行 func
		void each(const std::function<void(Value_t&)>& func)
		{
			auto It = super::derived().begin(), End = super::derived().end();

			while (It != End)
			{
				func(*It);
				++It;
			}
		}

		//  遍历每个元素, 并执行 func
		void each(const std::function<void(const Value_t&)>& func) const
		{
			auto It = super::derived().begin(), End = super::derived().end();

			while (It != End)
			{
				func(*It);
				++It;
			}
		}

		// 得到第一个满足条件的元素下标
		c_size index_if(const CheckTask& check, c_size pos = 0) const
		{
			auto It = super::derived().begin(), End = super::derived().end();
			std::advance(It, pos);
			while (It != End)
			{
				if (check(*It)) return pos;
				++It, ++pos;
			}
		}

		// 得到第一个相等的元素下标
		c_size index(const Value_t& v, c_size pos = 0) const
		{
			return index_if([&v](const Value_t& x) { return x == v; }, pos);
		}

		// 删除满足条件的元素, 返回删除的元素个数
		c_size pop_if(const CheckTask& check, c_size pos = 0)
		{
			auto l = std::next(super::derived().begin(), pos);
			// 找到第一个满足条件的元素
			while (l != super::derived().end() && !check(*l)) ++l;

			// 第一个满足条件后的第一个不满足条件的元素
			auto r = std::next(l, 1);

			// l 到 r 之间的元素都满足条件
			while (r != super::derived().end())
			{
				while (r != super::derived().end() && check(*r)) ++r;
				if (r == super::derived().end()) break;
				std::swap(*l, *r);
				++l, ++r;
			}

			c_size count = std::distance(l, r);
			super::derived().pop_back(count);
			return count;
		}

		c_size find(const Value_t& v) const
		{
			return index(v);
		}

		Iterator find_it(const Value_t& v)
		{
			auto it = super::derived().begin(), end_ = super::derived().end();
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
			auto it = super::derived().begin(), end_ = super::derived().end();
			while (it != end_)
			{
				if (*it == v)
					return it;

				++it;
			}

			return end_;
		}

		cmp_t __cmp__(const Derived& other) const
		{
			auto m_it = super::derived().begin(), m_end = super::derived().end();
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
			if (super::derived().size() != other.size())
				return false;

			return super::derived().__cmp__(other) == 0;
		}

		CString __str__() const
		{
			std::stringstream ss;
			ss << "[";
			for (auto it = super::derived().begin(), end_ = super::derived().end(); it != end_; ++it)
			{
				ss << *it;
				if (std::next(it) != end_)
					ss << ", ";
			}
			ss << "]";
			return ss.str();
		}

		Iterator begin() { return Iterator(this, 0); }

		Iterator end() { return Iterator(this, super::derived().size()); }

		ConstIterator begin() const { return ConstIterator(this, 0); }

		ConstIterator end() const { return ConstIterator(this, super::derived().size()); }
	};
}

#endif