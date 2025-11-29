#ifndef AYR_AIR_DICT_HPP
#define AYR_AIR_DICT_HPP

#include "Chain.hpp"
#include "Table.hpp"

namespace ayr
{
	/*
	* @brief 字典类
	*
	* @tparam K key类型，必须是可哈希的对象
	*
	* @tparam V value类型
	*
	* Dict使用robin算法哈希实现
	*
	* 字典的实现采用了哈希表(HashTable)和链表(Chain)的组合实现。
	*
	* 保证key-value的顺序性，key-value的迭代顺序为插入顺序。
	*/
	template<Hashable K, typename V>
	class Dict : public Object<Dict<K, V>>
	{
		using self = Dict<K, V>;
	public:
		using Key_t = K;

		using Value_t = V;

		using KV_t = std::pair<const Key_t, Value_t>;

		using Table_t = Table<typename Chain<KV_t>::Node_t*>;

		using TableValue_t = typename Chain<KV_t>::Node_t*;

		using Iterator = typename Chain<KV_t>::Iterator;

		using ConstIterator = typename Chain<KV_t>::ConstIterator;

		Dict() : htable_(), kv_chain_() {}

		Dict(c_size size) : htable_(size), kv_chain_() {}

		Dict(const std::initializer_list<KV_t>& il) : htable_(il.size()), kv_chain_()
		{
			for (auto& [key, value] : il)
				insert(key, value);
		}

		Dict(const self& other) : htable_(other.htable_), kv_chain_(other.kv_chain_) {}

		Dict(self&& other) noexcept : htable_(std::move(other.htable_)), kv_chain_(std::move(other.kv_chain_)) {}

		Dict& operator=(const self& other)
		{
			if (this == &other) return *this;
			ayr_destroy(this);
			return *ayr_construct(this, other);
		}

		Dict& operator=(self&& other) noexcept
		{
			if (this == &other) return *this;
			ayr_destroy(this);
			return *ayr_construct(this, std::move(other));
		}

		// key-value对的数量
		c_size size() const { return htable_.size(); }

		// 表是否为空
		bool empty() const { return htable_.empty(); }

		// 表容量
		c_size capacity() const { return htable_.capacity(); }

		// 判断是否包含key
		bool contains(const Key_t& key) const { return htable_.contains(ayrhash(key)); }

		// key的迭代对象
		auto keys() const { return std::views::keys(kv_chain_); }

		// value的迭代对象
		auto values() { return std::views::values(kv_chain_); }

		// value的迭代对象
		auto values() const { return std::views::values(kv_chain_); }

		// key-value对的迭代对象
		auto items() { return std::ranges::subrange(kv_chain_.begin(), kv_chain_.end()); }

		// key-value对的迭代对象
		auto items() const { return std::ranges::subrange(kv_chain_.begin(), kv_chain_.end()); }

		/*
		* @brief 根据key获取value, 若key不存在, 抛出异常
		*
		* @param key 要获取的key
		*
		* @return const Value_t& 要获取的value
		*/
		const Value_t& get(const Key_t& key) const
		{
			TableValue_t item = get_impl(ayrhash(key));
			if (item) return item->value.second;
			RuntimeError(std::format("Key '{}' not found in dictionary.", key));
			return None;
		}

		/*
		* @brief 根据key获取value, 若key不存在, 抛出异常
		*
		* @param key 要获取的key
		*
		* @return const Value_t& 要获取的value
		*/
		Value_t& get(const Key_t& key)
		{
			TableValue_t item = get_impl(ayrhash(key));
			if (item) return item->value.second;
			RuntimeError(std::format("Key '{}' not found in dictionary.", key));
			return None;
		}

		/*
		* @brief 根据key获取value, 若key不存在, 返回default_value
		*
		* @param key 要获取的key
		*
		* @param default_value 默认值
		*
		* @return const Value_t& 要获取的value
		*/
		const Value_t& get(const Key_t& key, const Value_t& default_value) const
		{
			TableValue_t item = get_impl(ayrhash(key));
			if (item) return item->value.second;
			return default_value;
		}

		/*
		* @brief 根据key获取value, 若key不存在, 返回default_value
		*
		* @param key 要获取的key
		*
		* @param default_value 默认值
		*
		* @return Value_t& 要获取的value
		*/
		Value_t& get(const Key_t& key, Value_t& default_value)
		{
			TableValue_t item = get_impl(ayrhash(key));
			if (item) return item->value.second;
			return default_value;
		}

		const Value_t& operator[](const Key_t& key) const { return get(key); }

		/*
		* @brief 根据key获取value, 若key不存在, 生成默认值
		*
		* @param key 要获取的key
		*
		* @return Value_t& 要获取的value
		*/
		template<DecaySameAs<Key_t> _K>
		Value_t& operator[](_K&& key)
		{
			hash_t hashv = ayrhash(key);

			auto [index, move_dist] = htable_.try_get(hashv);

			// 当前表中已有该key
			if (index_hashv_has_value(index, hashv))
				return htable_.items_[index].value()->value.second;

			auto kv_node = kv_chain_.append(std::forward<_K>(key), Value_t{});

			htable_.insert_value_on_index(index, hashv, move_dist, kv_node);
			return kv_node->value.second;
		}

		/*
		* @brief 向字典中插入一个key-value对, 若key已经存在, 则覆盖原有值
		*
		* @param key 要插入的key
		*
		* @param value 要插入的value
		*
		* @return Value_t& 被插入的value
		*/
		template<typename _K, typename _V>
		Value_t& insert(_K&& key, _V&& value)
		{
			return insert(std::forward<_K>(key),
				std::forward<_V>(value),
				ayrhash(Key_t{ key })
			);
		}

		/*
		* @brief 向字典中插入一个key-value对, 若key已经存在, 则覆盖原有值
		*
		* @param key 要插入的key
		*
		* @param value 要插入的value
		*
		* @param hashv 待插入的key的hash值
		*
		* @return Value_t& 被插入的value
		*/
		template<typename _K, typename _V>
		Value_t& insert(_K&& key, _V&& value, hash_t hashv)
		{
			auto [index, move_dist] = htable_.try_get(hashv);

			if (index_hashv_has_value(index, hashv))
				return htable_.items_[index].value()->value.second = std::forward<_V>(value);

			auto kv_node = kv_chain_.append(std::forward<_K>(key), std::forward<_V>(value));
			htable_.insert_value_on_index(index, hashv, move_dist, kv_node);

			return kv_node->value.second;
		}

		/*
		* @brief 向字典插入一个key-value对, 若key已经存在, 则无事发生
		*
		* @param key 要插入的key
		*
		* @param value 要插入的value
		*
		* @return key位置上的value
		*/
		template<typename _K, typename _V>
		Value_t& setdefault(_K&& key, _V&& default_value)
		{
			hash_t hashv = ayrhash(key);
			auto [index, move_dist] = htable_.try_get(hashv);
			if (index_hashv_has_value(index, hashv))
				return htable_.items_[index].value()->value.second;

			auto kv_node = kv_chain_.append(std::forward<_K>(key), std::forward<_V>(default_value));
			htable_.insert_value_on_index(index, hashv, move_dist, kv_node);
		}

		/*
		* @brief 根据key删除key-value
		*
		* @param key 要删除的key
		*/
		void pop(const Key_t& key)
		{
			hash_t hashv = ayrhash(key);
			auto [index, move_dist] = htable_.try_get(hashv);
			if (index_hashv_has_value(index, hashv))
			{
				kv_chain_.pop(htable_.items_[index].value());
				htable_.pop_value_on_index(index, hashv, move_dist);
			}
		}

		// 清空字典
		void clear() { htable_.clear(); kv_chain_.clear(); }

		/*
		* @brief 字典的并集
		*
		* @detail 两个字典都有的key，取当前字典的value
		*
		* @param other 另一个字典
		*
		* @return self 并集字典
		*/
		self operator&(const self& other) const
		{
			if (this == &other) return *this;
			self result(std::min(size(), other.size()));
			for (auto& [key, value] : items())
				if (other.contains(key))
					result.insert(key, value);

			return result;
		}

		self& operator&=(const self& other) { return *this = *this & other; }

		/*
		* @brief 字典的交集
		*
		* @detail 根据key有一方存在，都存在的取当前字典的value
		*
		* @param other 另一个字典
		*
		* @return self 交集字典
		*/
		self operator|(const self& other) const
		{
			if (this == &other) return *this;
			self result(*this);
			for (auto& [key, value] : other.items())
				if (!result.contains(key))
					result.insert(key, value);

			return result;
		}

		/*
		* @brief 字典的交集
		*
		* @detail 根据key有一方存在，都存在的取当前字典的value
		*
		* @param other 另一个字典
		*
		* @return self 交集字典
		*/
		self operator|(self&& other) const
		{
			if (this == &other) return *this;
			self result(*this);
			for (auto& [key, value] : other.items())
				if (!result.contains(key))
					result.insert(key, std::move(value));
			return result;
		}

		self& operator|=(const self& other) { return *this = *this | other; }

		self& operator|=(self&& other) { return *this = *this | std::move(other); }

		/*
		* @brief 字典的差集
		*
		* @detail 只有一个字典有key，且另一个字典没有key，有key的value
		*
		* @param other 另一个字典
		*
		* @return self 差集字典
		*/
		self operator^(const self& other) const
		{
			if (this == &other) return self();
			self result(size() + other.size());
			for (auto& [key, value] : items())
				if (!other.contains(key))
					result.insert(key, value);

			for (auto& [key, value] : other.items())
				if (!contains(key))
					result.insert(key, value);
			return result;
		}

		/*
		* @brief 字典的差集
		*
		* @detail 只有一个字典有key，且另一个字典没有key，有key的value
		*
		* @param other 另一个字典
		*
		* @return self 差集字典
		*/
		self operator^(self&& other) const
		{
			if (this == &other) return self();
			self result(size() + other.size());
			for (auto& [key, value] : items())
				if (!other.contains(key))
					result.insert(key, value);

			for (auto& [key, value] : other.items())
				if (!contains(key))
					result.insert(std::move(key), std::move(value));
			return result;
		}

		self& operator^= (const self& other) { return *this = *this ^ other; }

		self& operator^= (self&& other) { return *this = *this ^ std::move(other); }

		void __repr__(Buffer& buffer) const
		{
			buffer << "{";
			bool flag = false;
			for (auto& [key, value] : items())
			{
				if (flag)
					buffer << ", ";
				else
					flag = true;
				buffer << key << ": " << value;
			}
			buffer << "}";
		}

		bool __equals__(const self& other) const
		{
			if (this == &other) return true;
			if (size() != other.size()) return false;
			for (auto& [key, value] : items())
				if (!other.contains(key) || other.get(key) != value)
					return false;
			return true;
		}

		Iterator begin() { return kv_chain_.begin(); }

		Iterator end() { return kv_chain_.end(); }

		ConstIterator begin() const { return kv_chain_.begin(); }

		ConstIterator end() const { return kv_chain_.end(); }
	private:
		/*
		* @brief 尝试获得hahv对应的值
		*
		* @param hashv 待查找的hash值
		*
		* @return node*
		*/
		TableValue_t get_impl(hash_t hashv) const
		{
			auto [index, move_dist] = htable_.try_get(hashv);

			if (index_hashv_has_value(index, hashv))
				return htable_.items_[index].value();
			else
				return nullptr;
		}

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

		Table_t htable_;

		Chain<KV_t> kv_chain_;
	};
}
#endif // AYR_AIR_DICT_HPP