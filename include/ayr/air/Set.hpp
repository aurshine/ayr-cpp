#ifndef AYR_AIR_SET_HPP
#define AYR_AIR_SET_HPP

#include "Chain.hpp"
#include "Table.hpp"

namespace ayr
{
	/*
	* @brief 集合类
	*
	* @tparam T 集合元素类型，必须是可哈希类型
	*
	* Set使用robin算法哈希实现
	*
	* 保证value的顺序性，value的迭代顺序为插入顺序
	*/
	template<Hashable T>
	class Set
	{
		using self = Set<T>;

		using Table_t = Table<typename Chain<T>::Node_t*>;

		Chain<T> chain_;

		Table_t htable_;
	public:
		using Value_t = T;

		using Iterator = typename Chain<T>::Iterator;

		using ConstIterator = typename Chain<T>::ConstIterator;

		Set() : htable_(), chain_() {}

		Set(c_size size) : htable_(size), chain_() {}

		Set(const std::initializer_list<Value_t>& il) : htable_(il.size()), chain_()
		{
			for (auto& v : il)
				insert(v);
		}

		Set(const Set& other) :htable_(other.htable_), chain_(other.chain_) {}

		Set(Set&& other) noexcept : htable_(std::move(other.htable_)), chain_(std::move(other.chain_)) {}

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

		bool empty() const { return htable_.empty(); }

		c_size capacity() const { return htable_.capacity(); }

		bool contains(const Value_t& value) const { return htable_.contains(ayrhash(value)); }

		/*
		* @brief 插入元素
		*
		* @param value 待插入元素
		*
		* 如果元素已经存在，则不插入，返回已存在元素的引用
		*/
		template<typename... Args>
		Value_t& insert(Args&&... args)
		{
			Value_t value(std::forward<Args>(args)...);
			hash_t hashv = ayrhash(value);
			auto [index, move_dist] = htable_.try_get(hashv);
			if (index_hashv_has_value(index, hashv))
				return htable_.items_[index].value()->value;

			auto node = chain_.append(std::move(value));
			htable_.insert_value_on_index(index, hashv, move_dist, node);
			return node->value;
		}

		/*
		* @brief 删除元素
		*
		* @param value 待删除元素
		*/
		void pop(const Value_t& value)
		{
			hash_t hashv = ayrhash(value);
			auto [index, move_dist] = htable_.try_get(hashv);
			if (index_hashv_has_value(index, hashv))
			{
				chain_.pop(htable_.items_[index].value());
				htable_.pop_value_on_index(index, hashv, move_dist);
			}
		}

		void clear() { htable_.clear(); chain_.clear(); }

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

		bool operator==(const self& other)
		{
			if (size() != other.size())
				return false;

			for (auto v : *this)
				if (!other.contains(v))
					return false;

			return true;
		}

		void __repr__(Buffer& buffer) const
		{
			buffer << "{";
			bool flag = false;
			for (auto& v : *this)
			{
				if (flag)
					buffer << ", ";
				else
					flag = true;

				buffer << v;
			}
			buffer << "}";
		}

		Iterator begin() { return chain_.begin(); }

		Iterator end() { return chain_.end(); }

		ConstIterator begin() const { return chain_.begin(); }

		ConstIterator end() const { return chain_.end(); }

	private:
		/*
		* @brief 根据index和hahv判断是否有预期值
		*
		* @param index 索引
		*
		* @param hashv 待查找的hash值
		*/
		bool index_hashv_has_value(c_size index, hash_t hashv) const
		{
			return htable_.items_[index].used() && htable_.items_[index].hashv == hashv;
		}
	};

	template<typename T, IteratorU<T> It>
	def set(const It& begin_, const It& end_) -> Set<T>
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