#ifndef AYR_SER_HPP
#define AYR_SER_HPP

#include <algorithm>

#include "Chain.hpp"
#include "base/Table.hpp"

namespace ayr
{
	template<Hashable T>
	class Set : public Object<Set<T>>
	{
		using self = Set<T>;

		using Table_t = Table<T>;
	public:
		using Value_t = T;

		using Iterator = Table_t::Iterator;

		using ConstIterator = Table_t::ConstIterator;

		Set() : htable_() {}

		Set(c_size size) : htable_(size) {}

		Set(const std::initializer_list<Value_t>& il) : htable_(il.size())
		{
			for (auto& v : il)
				insert(v);
		}

		Set(const Set& other) :htable_(other.htable_) {}

		Set(Set&& other) noexcept : htable_(std::move(other.htable_)) {}

		Set& operator=(const Set& other)
		{
			if (this == &other) return *this;
			ayr_destroy(this);
			return *ayr_construct(this, other);
		}

		Set& operator=(Set&& other) noexcept
		{
			if (this == &other) return *this;
			ayr_destroy(this);
			return *ayr_construct(this, std::move(other));
		}

		c_size size() const { return htable_.size(); }

		c_size capacity() const { return htable_.capacity(); }

		bool contains(const Value_t& value) const { return htable_.contains(ayrhash(value)); }

		template<typename _V>
		void insert(_V&& value)
		{
			if (contains(value)) return;
			Value_t v = std::forward<_V>(value);
			htable_.insert(ayrhash(v), std::move(v));
		}

		void pop(const Value_t& value) { htable_.pop(ayrhash(value)); }

		self operator& (const self& other) const
		{
			if (this == &other) return *this;
			if (size() < other.size())
			{
				self result(size());
				for (auto& v : *this)
					if (other.contains(v))
						result.insert(v);
				return result;
			}
			else
			{
				self result(other.size());
				for (auto& v : other)
					if (contains(v))
						result.insert(v);
				return result;
			}
		}

		self& operator&=(const self& other) { return *this = *this & other; }

		self operator|(const self& other) const
		{
			if (this == &other) return *this;
			self result(*this);
			for (auto& v : other)
				result.insert(v);
			return result;
		}

		self operator|(self&& other) const
		{
			if (this == &other) return *this;
			self result(*this);
			for (auto& v : other)
				result.insert(std::move(v));
			return result;
		}

		self& operator|=(const self& other) { return *this = *this | other; }

		self& operator|=(self&& other) { return *this = *this | std::move(other); }

		self operator^ (const self& other) const
		{
			if (this == &other) return self();

			self result(size() + other.size());
			for (auto& v : *this)
				if (!other.contains(v))
					result.insert(v);

			for (auto& v : other)
				if (!contains(v))
					result.insert(v);

			return result;
		}

		self operator^(self&& other) const
		{
			if (this == &other) return self();

			self result(size() + other.size());
			for (auto& v : *this)
				if (!other.contains(v))
					result.insert(v);

			for (auto& v : other)
				if (!contains(v))
					result.insert(std::move(v));

			return result;
		}

		self& operator^=(const self& other) { return *this = *this ^ other; }

		self& operator^=(self&& other) { return *this = *this ^ std::move(other); }

		void clear() { htable_.clear(); }

		CString __str__() const
		{
			DynArray<CString> strs;
			strs.append("{");
			for (auto& v : *this)
			{
				strs.append(cstr(v));
				strs.append(", ");

			}
			if (size() > 0) strs.pop_back();
			strs.append("}");

			return cjoin(strs);
		}

		bool __equals__(const self& other)
		{
			if (size() != other.size())
				return false;

			for (auto v : *this)
				if (!other.contains(v))
					return false;

			return true;
		}

		void __swap__(self& other) { ayr::swap(htable_, other.htable_); }

		Iterator begin() { return htable_.begin(); }

		Iterator end() { return htable_.end(); }

		ConstIterator begin() const { return htable_.begin(); }

		ConstIterator end() const { return htable_.end(); }
	private:
		Table_t htable_;
	};

	template<typename T, IteratorU<T> It>
	def set(const It& begin_, const It& end_)
	{
		Set<T> r_set(std::distance(begin_, end_));
		It walker = begin_;

		while (walker != end_)
		{
			r_set.insert(*walker);
			++walker;
		}
		return r_set;
	}

	template<typename T, IteratorU<T> It>
	def set(It&& begin_, It&& end_)
	{
		Set<T> r_set(std::distance(begin_, end_));
		It walker = begin_;

		while (walker != end_)
		{
			r_set.insert(std::move(*walker));
			++walker;
		}
		return r_set;
	}

	template<typename T, IteratableU<T> Obj>
	def set(const Obj& elems) { return set<T>(elems.begin(), elems.end()); }

	template<typename T, IteratableU<T> Obj>
	def set(Obj&& elems) { return set<T>(std::move(elems.begin()), std::move(elems.end())); }
}
#endif