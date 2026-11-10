#ifndef AYR_AIR_DICT_HPP
#define AYR_AIR_DICT_HPP

#include "Chain.hpp"
#include "Optional.hpp"
#include "Table.hpp"

namespace ayr
{
	/**
	 * @brief 字典哈希表中的业务表项，保存键值节点指针及缓存哈希。
	 * @tparam K 键类型。
	 * @tparam V 值类型。
	 * @details 不拥有节点；节点由 Dict 的链表管理，移动表项不会移动节点中的键值。
	 */
	template<typename K, typename V>
	class DictItem
	{
		using self = DictItem<K, V>;

		// 节点中键的缓存哈希，供查询和扩容重排使用。
		hash_t hashv_;
	public:
		using Node_t = typename Chain<std::pair<const K, V>>::Node_t;

		// 非拥有指针；访问键值前必须指向有效节点。
		Node_t* node;

		using Key = K;

		using Val = V;

		/** @brief 保存节点指针及哈希；默认构造时不关联节点。 */
		constexpr DictItem(Node_t* node = nullptr, hash_t hashv = 0) : node(node), hashv_(hashv) {}

		/** @brief 转移节点关联和缓存哈希，并清空来源表项。 */
		constexpr DictItem(self&& other) noexcept : node(other.node), hashv_(other.hashv_)
		{
			other.node = nullptr;
			other.hashv_ = 0;
		}

		/** @brief 转移节点关联和缓存哈希，不释放原关联节点，支持自赋值。 */
		self& operator=(self&& other)
		{
			if (this == &other) return *this;
			node = std::exchange(other.node, nullptr);
			hashv_ = std::exchange(other.hashv_, 0);
			return *this;
		}

		/** @brief 返回有效节点中的只读键。 */
		constexpr const Key& key() const { return node->value.first; }

		/** @brief 返回有效节点中的可修改值。 */
		constexpr Val& value() { return node->value.second; }

		/** @brief 返回有效节点中的只读值。 */
		constexpr const Val& value() const { return node->value.second; }

		/** @brief 比较有效节点中的键与给定键是否相等。 */
		constexpr bool key_equal(const Key& key) const { return this->key() == key; }

		/** @brief 返回缓存的键哈希，无需访问链表节点。 */
		constexpr hash_t hashv() const { return hashv_; }
	};

	/**
	 * @brief 按键值对的插入顺序迭代的字典。
	 * @tparam K 可哈希、可比较相等的键类型。
	 * @tparam V 值类型。
	 * @details 链表拥有键值节点，Robin Hood 哈希表保存节点关联及缓存哈希。
	 * 通过哈希和键共同判断命中；覆盖已有值不改变迭代顺序。
	 * 扩容只重排表项，不移动链表节点，因此不使已有节点的引用和迭代器失效。
	 */
	template<Hashable K, typename V>
	class Dict
	{
		using self = Dict<K, V>;

		using Item = DictItem<K, V>;

		using Table_t = Table<Item>;

		using KeyVal = std::pair<const typename Item::Key, typename Item::Val>;

		// 按键索引链表节点，不拥有节点。
		Table_t htable_;

		// 拥有键值节点，并维护插入顺序。
		Chain<KeyVal> kv_chain_;
	public:
		using Key = typename Item::Key;

		using Val = typename Item::Val;

		using Iterator = typename Chain<KeyVal>::Iterator;

		using ConstIterator = typename Chain<KeyVal>::ConstIterator;

		/** @brief 构造使用最小哈希表容量的空字典。 */
		Dict() : htable_(), kv_chain_() {}

		/** @brief 根据预期元素数量预分配哈希表，字典初始仍为空。 */
		Dict(c_size size) : htable_(size), kv_chain_() {}

		/** @brief 按列表顺序插入键值对；重复键覆盖值并保留首次插入的位置。 */
		Dict(const std::initializer_list<KeyVal>& il) : Dict(il.size())
		{
			for (const auto& [key, value] : il)
				insert(key, value);
		}

		/** @brief 按来源顺序复制键值节点，并重建指向新节点的哈希表。 */
		Dict(const self& other) : Dict(other.size())
		{
			for (const auto& [key, value] : other.items())
				insert(key, value);
		}

		/** @brief 一并接管哈希表和链表，保留表项与节点的对应关系。 */
		Dict(self&& other) noexcept : htable_(std::move(other.htable_)), kv_chain_(std::move(other.kv_chain_)) {}

		/** @brief 释放当前内容，再复制来源字典；支持自赋值。 */
		Dict& operator=(const self& other)
		{
			if (this == &other) return *this;
			ayr_destroy(this);
			return *ayr_construct(this, other);
		}

		/** @brief 释放当前内容，再接管来源字典；支持自移动赋值。 */
		Dict& operator=(self&& other) noexcept
		{
			if (this == &other) return *this;
			ayr_destroy(this);
			return *ayr_construct(this, std::move(other));
		}

		/** @brief 返回键值对的数量。 */
		c_size size() const { return htable_.size(); }

		/** @brief 判断字典是否为空。 */
		bool empty() const { return htable_.empty(); }

		/** @brief 返回底层哈希表的槽位容量。 */
		c_size capacity() const { return htable_.capacity(); }

		/** @brief 判断是否存在与 key 相等的键。 */
		bool contains(const Key& key) const { return htable_.contains(key, ayrhash(key)); }

		/** @brief 返回按插入顺序访问键的只读视图。 */
		auto keys() const { return std::views::keys(kv_chain_); }

		/** @brief 返回按插入顺序访问值的可修改视图。 */
		auto values() { return std::views::values(kv_chain_); }

		/** @brief 返回按插入顺序访问值的只读视图。 */
		auto values() const { return std::views::values(kv_chain_); }

		/** @brief 返回键值对视图；键只读，值可修改，顺序与插入顺序一致。 */
		auto items() { return std::ranges::subrange(kv_chain_.begin(), kv_chain_.end()); }

		/** @brief 返回按插入顺序访问键值对的只读视图。 */
		auto items() const { return std::ranges::subrange(kv_chain_.begin(), kv_chain_.end()); }

		/**
		 * @brief 返回指定键对应的只读值；键不存在时抛出异常。
		 * @param key 要查找的键。
		 * @return 已存储值的常量引用。
		 */
		const Val& get(const Key& key) const
		{
			hash_t hashv = ayrhash(key);
			auto [index, move_dist, found] = htable_.probe(key, hashv);

			if (found)
				return htable_.items_[index].value();

			RuntimeError(fmt("Key '{}' not found in dictionary.", key));
			return None;
		}

		/**
		 * @brief 返回指定键对应的可修改值；键不存在时抛出异常。
		 * @param key 要查找的键。
		 * @return 已存储值的引用。
		 */
		Val& get(const Key& key)
		{
			hash_t hashv = ayrhash(key);
			auto [index, move_dist, found] = htable_.probe(key, hashv);

			if (found)
				return htable_.items_[index].value();

			RuntimeError(fmt("Key '{}' not found in dictionary.", key));
			return None;
		}

		/**
		 * @brief 查询只读值，键不存在时返回默认值，不插入新键。
		 * @param key 要查找的键。
		 * @param default_value 键缺失时返回的默认值。
		 * @return 已存储值或 default_value 的常量引用。
		 * @note 返回默认值时，引用的有效期取决于 default_value 的生命周期。
		 */
		const Val& get(const Key& key, const Val& default_value) const
		{
			hash_t hashv = ayrhash(key);
			auto [index, move_dist, found] = htable_.probe(key, hashv);

			if (found)
				return htable_.items_[index].value();
			return default_value;
		}

		/**
		 * @brief 查询可修改值，键不存在时返回默认值，不插入新键。
		 * @param key 要查找的键。
		 * @param default_value 键缺失时返回的默认值。
		 * @return 已存储值或 default_value 的引用。
		 * @note 返回默认值时，引用的有效期取决于 default_value 的生命周期。
		 */
		Val& get(const Key& key, Val& default_value)
		{
			hash_t hashv = ayrhash(key);
			auto [index, move_dist, found] = htable_.probe(key, hashv);

			if (found)
				return htable_.items_[index].value();
			return default_value;
		}

		/** @brief 查询只读值，键不存在时抛出异常；键参数的退化类型须与 Key 相同。 */
		template<DecaySameAs<Key> _K>
		const Val& operator[](const _K& key) const { return get(key); }

		/**
		 * @brief 查询可修改值，键不存在时在末尾插入该键和默认构造的值。
		 * @param key 要查找或插入的键，退化类型须与 Key 相同。
		 * @return 已存储值或新插入值的引用。
		 * @note Val 必须支持默认构造；已有键的值保持不变。
		 */
		template<DecaySameAs<Key> _K>
		Val& operator[](_K&& key) 
		{ 
			hash_t hashv = ayrhash<Key>(key);
			auto [index, move_dist, found] = htable_.probe(key, hashv);
			if (found)
				return htable_.items_[index].value();

			auto node = kv_chain_.append(std::forward<_K>(key), Val{});
			htable_.insert_value_on_index(index, move_dist, Item(node, hashv));
			return node->value.second;
		}

		/**
		 * @brief 插入键值对；已有键覆盖值并保持位置，新键追加到末尾。
		 * @param key 要插入的键，退化类型须与 Key 相同。
		 * @param value 要保存的值，按参数的值类别转发。
		 * @return 插入或覆盖后字典中值的引用。
		 */
		template<DecaySameAs<Key> _K, std::convertible_to<Val> _V>
		Val& insert(_K&& key, _V&& value)
		{
			hash_t hashv = ayrhash<Key>(key);
			auto [index, move_dist, found] = htable_.probe(key, hashv);
			if (found)
				return htable_.items_[index].value() = std::forward<_V>(value);

			auto node = kv_chain_.append(std::forward<_K>(key), std::forward<_V>(value));
			htable_.insert_value_on_index(index, move_dist, Item(node, hashv));
			return node->value.second;
		}

		/**
		 * @brief 仅当键不存在时，在末尾插入给定键值对。
		 * @param key 要查找或插入的键，退化类型须与 Key 相同。
		 * @param value 键缺失时保存的值，按参数的值类别转发。
		 * @return 该键对应的已存储值或新插入值的引用。
		 * @note 键已存在时，不覆盖原值，也不移动 value 参数。
		 */
		template<DecaySameAs<Key> _K, std::convertible_to<Val> _V>
		Val& setdefault(_K&& key, _V&& value) 
		{ 
			hash_t hashv = ayrhash<Key>(key);
			auto [index, move_dist, found] = htable_.probe(key, hashv);
			if (found)
				return htable_.items_[index].value();

			auto node = kv_chain_.append(std::forward<_K>(key), std::forward<_V>(value));
			htable_.insert_value_on_index(index, move_dist, Item(node, hashv));
			return node->value.second;
		}

		/**
		 * @brief 删除指定键值对；键不存在时保持不变。
		 * @param key 要删除的键。
		 * @note 仅使被删除节点的引用和迭代器失效，其余节点的相对顺序不变。
		 */
		void pop(const Key& key)
		{
			hash_t hashv = ayrhash(key);
			auto [index, move_dist, found] = htable_.probe(key, hashv);
			if (found)
			{
				// 先保存节点并移除表项，避免槽位回移后丢失待释放的节点关联。
				auto node = htable_.items_[index].node;
				htable_.pop_value_on_index(index);
				kv_chain_.pop(node);
			}
		}

		/** @brief 清空哈希表并释放所有键值节点，将槽位容量恢复到最小值。 */
		void clear() { htable_.clear(); kv_chain_.clear(); }

		/**
		 * @brief 按键求交集，值及迭代顺序均取自当前字典。
		 * @param other 参与求交集的字典。
		 * @return 仅包含双方共有键的新字典。
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

		/** @brief 将当前字典替换为按键求得的交集。 */
		self& operator&=(const self& other) { return *this = *this & other; }

		/**
		 * @brief 按键求并集，重复键保留当前字典中的值。
		 * @param other 参与求并集的字典。
		 * @return 先保留当前字典顺序，再按 other 的顺序追加其独有键的新字典。
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

		/**
		 * @brief 按键求并集，复制当前字典并移动 other 中独有键的值。
		 * @param other 值可以被移出的来源字典，来源节点和键仍保留。
		 * @return 顺序和重复键处理规则与常量引用版本相同的新字典。
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

		/** @brief 将当前字典替换为并集，重复键保留当前值。 */
		self& operator|=(const self& other) { return *this = *this | other; }

		/** @brief 将当前字典替换为并集，并移动来源字典中独有键的值。 */
		self& operator|=(self&& other) { return *this = *this | std::move(other); }

		/**
		 * @brief 按键求对称差，只保留恰好出现在一方的键值对。
		 * @param other 参与求对称差的字典。
		 * @return 先按当前字典顺序收集独有键，再按 other 顺序收集独有键的新字典。
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

		/**
		 * @brief 按键求对称差，复制当前字典独有项并移动 other 独有项的值。
		 * @param other 值可以被移出的来源字典；节点内的 const 键仍通过复制构造。
		 * @return 顺序与常量引用版本相同的对称差字典。
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

		/** @brief 将当前字典替换为按键求得的对称差。 */
		self& operator^= (const self& other) { return *this = *this ^ other; }

		/** @brief 将当前字典替换为对称差，并移动来源字典独有项的值。 */
		self& operator^= (self&& other) { return *this = *this ^ std::move(other); }

		/** @brief 比较键值对内容是否相同，不要求插入顺序相同。 */
		bool operator==(const self& other) const
		{
			if (this == &other) return true;
			if (size() != other.size()) return false;
			for (auto& [key, value] : items())
				if (!other.contains(key) || other.get(key) != value)
					return false;
			return true;
		}

		/** @brief 按插入顺序输出字典中的键值对。 */
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

		/** @brief 返回首个键值节点的迭代器，键只读、值可修改。 */
		Iterator begin() { return kv_chain_.begin(); }

		/** @brief 返回尾后迭代器。 */
		Iterator end() { return kv_chain_.end(); }

		/** @brief 返回首个键值节点的只读迭代器。 */
		ConstIterator begin() const { return kv_chain_.begin(); }

		/** @brief 返回只读尾后迭代器。 */
		ConstIterator end() const { return kv_chain_.end(); }
	};
}
#endif // AYR_AIR_DICT_HPP
