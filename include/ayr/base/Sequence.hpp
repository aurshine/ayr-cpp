#ifndef AYR_BASE_SEQUENCE_HPP
#define AYR_BASE_SEQUENCE_HPP

#include <functional>

#include "itertools/IndexIterator.hpp"
#include "raise_error.hpp"

namespace ayr
{
	// 需要子类实现 size() 和 at()
	template<typename Derived, typename T>
	class Sequence
	{
	public:
		using self = Sequence<Derived, T>;
	public:
		using Value_t = T;

		using Iterator = IndexIterator<false, self, Value_t>;

		using ConstIterator = IndexIterator<true, self, Value_t>;

		using CheckTask = std::function<bool(const Value_t&)>;

		const Value_t& at(c_size index) const { NotImplementedError(std::format("{} Not implemented at(c_size)", dtype(Derived))); return None; }

		c_size size() const { NotImplementedError(std::format("{} Not implemented size()", dtype(Derived))); return None; }

		bool empty() const { return derived().size() == 0; }

		// 删除最后n个元素
		void pop_back(c_size n = 1) { NotImplementedError(std::format("{} Not implemented pop_back(c_size)", dtype(Derived))); }

		Value_t& front() { return *derived().begin(); }

		const Value_t& front() const { return *derived().begin(); }

		Value_t& back() { return *std::next(derived().begin(), derived().size() - 1); }

		const Value_t& back() const { return *std::next(derived().begin(), derived().size() - 1); }

		Value_t& operator[] (c_size index) { return derived().at(neg_index(index, derived().size())); }

		const Value_t& operator[] (c_size index) const { return derived().at(neg_index(index, derived().size())); }

		bool contains(const Value_t& v) const { return find_it(v) != derived().end(); }

		//  遍历每个元素, 并执行 func
		void each(const std::function<void(Value_t&)>& func)
		{
			auto It = derived().begin(), End = derived().end();

			while (It != End)
			{
				func(*It);
				++It;
			}
		}

		//  遍历每个元素, 并执行 func
		void each(const std::function<void(const Value_t&)>& func) const
		{
			auto It = derived().begin(), End = derived().end();

			while (It != End)
			{
				func(*It);
				++It;
			}
		}

		// 得到第一个满足条件的元素下标
		c_size index_if(const CheckTask& check, c_size pos = 0) const
		{
			auto It = derived().begin(), End = derived().end();
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
			auto l = std::next(derived().begin(), pos);
			// 找到第一个满足条件的元素
			while (l != derived().end() && !check(*l)) ++l;

			// 第一个满足条件后的第一个不满足条件的元素
			auto r = std::next(l, 1);

			// l 到 r 之间的元素都满足条件
			while (r != derived().end())
			{
				while (r != derived().end() && check(*r)) ++r;
				if (r == derived().end()) break;
				std::swap(*l, *r);
				++l, ++r;
			}

			c_size count = std::distance(l, r);
			derived().pop_back(count);
			return count;
		}

		c_size find(const Value_t& v) const
		{
			return index(v);
		}

		Iterator find_it(const Value_t& v)
		{
			auto it = derived().begin(), end_ = derived().end();
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
			auto it = derived().begin(), end_ = derived().end();
			while (it != end_)
			{
				if (*it == v)
					return it;

				++it;
			}

			return end_;
		}

		std::strong_ordering operator<=>(const Derived& other) const
		{
			auto m_it = derived().begin(), m_end = derived().end();
			auto o_it = other.begin(), o_end = other.end();
			while (m_it != m_end && o_it != o_end)
			{
				if (*m_it != *o_it) return *m_it <=> *o_it;
				++m_it, ++o_it;
			}

			if (m_it != m_end) return std::strong_ordering::greater;
			if (o_it != o_end) return std::strong_ordering::less;
			return std::strong_ordering::equal;
		}

		bool operator==(const Derived& other) const
		{
			if (derived().size() != other.size())
				return false;
			auto m_it = derived().begin(), m_end = derived().end();
			auto o_it = other.begin(), o_end = other.end();
			while (m_it != m_end && o_it != o_end)
			{
				if (*m_it != *o_it) return false;
				++m_it, ++o_it;
			}
			return m_it == m_end && o_it == o_end;
		}

		void __repr__(Buffer& buffer) const
		{
			buffer << "[";
			for (auto it = derived().begin(), end_ = derived().end(); it != end_; ++it)
			{
				buffer << *it;
				if (std::next(it) != end_)
					buffer << ", ";
			}
			buffer << "]";
		}

		Iterator begin() { return Iterator(this, 0); }

		Iterator end() { return Iterator(this, derived().size()); }

		ConstIterator begin() const { return ConstIterator(this, 0); }

		ConstIterator end() const { return ConstIterator(this, derived().size()); }
	private:
		Derived& derived() { return static_cast<Derived&>(*this); }

		const Derived& derived() const { return static_cast<const Derived&>(*this); }
	};
}

#endif