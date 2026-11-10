#ifndef AYR_AIR_SET_HPP
#define AYR_AIR_SET_HPP

#include "Chain.hpp"
#include "Table.hpp"

namespace ayr
{
	/**
	 * @brief 集合哈希表中的业务表项，保存元素节点指针及缓存哈希。
	 * @tparam V 元素类型，同时作为查找键和值。
	 * @details 不拥有节点；节点由 Set 的链表管理，移动表项不会移动节点中的元素。
	 */
	template<typename V>
	class SetItem
	{
		using self = SetItem<V>;

		using Node_t = typename Chain<V>::Node_t;
		
		// 节点中元素的缓存哈希，供查询和扩容重排使用。
		hash_t hashv_;
	public:
		// 非拥有指针；访问键值前必须指向有效节点。
		Node_t* node;

		using Key = V;

		using Val = V;

		/** @brief 保存节点指针及哈希；默认构造时不关联节点。 */
		constexpr SetItem(Node_t* node = nullptr, hash_t hashv = 0) : node(node), hashv_(hashv) {}

		/** @brief 转移节点关联和缓存哈希，并清空来源表项。 */
		constexpr SetItem(self&& other) noexcept : node(other.node), hashv_(other.hashv_)
		{
			other.node= nullptr;
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

		/** @brief 返回有效节点中的元素，作为查找键。 */
		constexpr Key& key() { return node->value; }

		/** @brief 返回有效节点中的只读查找键。 */
		constexpr const Key& key() const { return node->value; }

		/** @brief 返回有效节点中的元素，与 key() 引用同一对象。 */
		constexpr Val& value() { return node->value; }

		/** @brief 返回有效节点中的只读元素。 */
		constexpr const Val& value() const { return node->value; }

		/** @brief 比较有效节点中的元素与给定键是否相等。 */
		constexpr bool key_equal(const Key& key) const { return this->key() == key; }

		/** @brief 返回缓存的元素哈希，无需访问链表节点。 */
		constexpr hash_t hashv() const { return hashv_; }
	};

	/**
	 * @brief 按元素的插入顺序迭代、通过相等比较去重的集合。
	 * @tparam T 可哈希、可比较相等的元素类型。
	 * @details 链表拥有元素节点，Robin Hood 哈希表保存节点关联及缓存哈希。
	 * 通过哈希和元素相等共同判断命中；插入重复元素不改变原元素及其位置。
	 * 扩容只重排表项，不移动链表节点，因此不使已有节点的引用和迭代器失效。
	 * @note 元素存储期间，不应通过可修改引用改变影响哈希或相等比较的内容。
	 */
	template<Hashable T>
	class Set
	{
		using self = Set<T>;

		using Item = SetItem<T>;

		using Table_t = Table<Item>;

		// 拥有元素节点，并维护插入顺序。
		Chain<T> chain_;

		// 按元素索引链表节点，不拥有节点。
		Table_t htable_;
	public:
		using Val = T;

		using Iterator = typename Chain<T>::Iterator;

		using ConstIterator = typename Chain<T>::ConstIterator;

		/** @brief 构造使用最小哈希表容量的空集合。 */
		Set() : htable_(), chain_() {}

		/** @brief 根据预期元素数量预分配哈希表，集合初始仍为空。 */
		Set(c_size size) : htable_(size), chain_() {}

		/** @brief 按列表顺序插入并去重，保留相等元素首次出现的位置。 */
		Set(const std::initializer_list<Val>& il) : Set(il.size())
		{
			for (const auto& v : il)
				insert(v);
		}

		/** @brief 按来源顺序复制元素节点，并重建指向新节点的哈希表。 */
		Set(const Set& other) : Set(other.size())
		{
			for (const auto& v : other)
				insert(v);
		}

		/** @brief 一并接管哈希表和链表，保留表项与节点的对应关系。 */
		Set(Set&& other) noexcept : htable_(std::move(other.htable_)), chain_(std::move(other.chain_)) {}

		/** @brief 释放当前内容，再复制来源集合；支持自赋值。 */
		Set& operator=(const Set& other)
		{
			if (this == &other) return *this;
			ayr_destroy(this);
			return *ayr_construct(this, other);
		}

		/** @brief 释放当前内容，再接管来源集合；支持自移动赋值。 */
		Set& operator=(Set&& other) noexcept
		{
			if (this == &other) return *this;
			ayr_destroy(this);
			return *ayr_construct(this, std::move(other));
		}

		/** @brief 返回去重后的元素数量。 */
		c_size size() const { return htable_.size(); }

		/** @brief 判断集合是否为空。 */
		bool empty() const { return htable_.empty(); }

		/** @brief 返回底层哈希表的槽位容量。 */
		c_size capacity() const { return htable_.capacity(); }

		/** @brief 判断是否存在与 value 相等的元素。 */
		bool contains(const Val& value) const { return htable_.contains(value, ayrhash(value)); }

		/**
		 * @brief 插入元素；相等元素已存在时保留原元素及其位置。
		 * @param value 按值接收的候选元素，新元素会移入末尾节点。
		 * @return 已存在元素或新插入元素的引用。
		 * @note 左值实参会先复制到 value，即使最终判定为重复元素。
		 */
		template<std::convertible_to<Val> _V>
		Val& insert(_V&& value)
		{
			hash_t hashv = ayrhash<Val>(value);
			auto [index, move_dist, found] = htable_.probe(value, hashv);

			if (found)
				return htable_.items_[index].value();

			auto node = chain_.append(std::move(value));
			htable_.insert_value_on_index(index, move_dist, Item(node, hashv));
			return node->value;
		}

		/**
		 * @brief 删除相等元素；元素不存在时保持不变。
		 * @param value 要删除的元素。
		 * @note 仅使被删除节点的引用和迭代器失效，其余节点的相对顺序不变。
		 */
		void pop(const Val& value)
		{
			hash_t hashv = ayrhash(value);
			auto [index, move_dist, found] = htable_.probe(value, hashv);
			if (found)
			{
				// 先保存节点并移除表项，避免槽位回移后丢失待释放的节点关联。
				auto node = htable_.items_[index].node;
				htable_.pop_value_on_index(index);
				chain_.pop(node);
			}
		}

		/** @brief 清空哈希表并释放所有元素节点，将槽位容量恢复到最小值。 */
		void clear() { htable_.clear(); chain_.clear(); }

		/**
		 * @brief 求集合交集。
		 * @return 包含共有元素的新集合，按较小集合的顺序排列；大小相同时取 other 的顺序。
		 */
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

		/** @brief 将当前集合替换为双方的交集。 */
		self& operator&=(const self& other) { return *this = *this & other; }

		/** @brief 求并集，保留当前集合顺序，再按 other 的顺序追加其独有元素。 */
		self operator|(const self& other) const
		{
			if (this == &other) return *this;
			self result(*this);
			for (auto& v : other)
				result.insert(v);
			return result;
		}

		/**
		 * @brief 复制当前集合，再移动 other 的候选元素求并集。
		 * @note other 的元素仍保留在来源节点中，但可能处于移后状态。
		 */
		self operator|(self&& other) const
		{
			if (this == &other) return *this;
			self result(*this);
			for (auto& v : other)
				result.insert(std::move(v));
			return result;
		}

		/** @brief 将当前集合替换为双方的并集。 */
		self& operator|=(const self& other) { return *this = *this | other; }

		/** @brief 将当前集合替换为并集，来源集合的元素可能被移出。 */
		self& operator|=(self&& other) { return *this = *this | std::move(other); }

		/** @brief 求对称差，先收集当前集合的独有元素，再收集 other 的独有元素。 */
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

		/**
		 * @brief 求对称差，复制当前集合的独有元素，并移动 other 的独有元素。
		 * @note 结果顺序与常量引用版本相同；other 的独有元素可能处于移后状态。
		 */
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

		/** @brief 将当前集合替换为双方的对称差。 */
		self& operator^=(const self& other) { return *this = *this ^ other; }

		/** @brief 将当前集合替换为对称差，来源集合的独有元素可能被移出。 */
		self& operator^=(self&& other) { return *this = *this ^ std::move(other); }

		/** @brief 比较双方是否包含相同元素，不要求插入顺序相同。 */
		bool operator==(const self& other) const
		{
			if (size() != other.size())
				return false;

			for (const auto& v : *this)
				if (!other.contains(v))
					return false;

			return true;
		}

		/** @brief 按插入顺序输出集合中的元素。 */
		void __repr__(Buffer& buffer) const
		{
			buffer << "{";
			bool flag = false;
			for (const auto& v : *this)
			{
				if (flag)
					buffer << ", ";
				else
					flag = true;

				buffer << v;
			}
			buffer << "}";
		}

		/** @brief 返回首个元素的迭代器；不得借此改变影响哈希或相等比较的内容。 */
		Iterator begin() { return chain_.begin(); }

		/** @brief 返回尾后迭代器。 */
		Iterator end() { return chain_.end(); }

		/** @brief 返回首个元素的只读迭代器。 */
		ConstIterator begin() const { return chain_.begin(); }

		/** @brief 返回只读尾后迭代器。 */
		ConstIterator end() const { return chain_.end(); }
	};

	/** @brief 从迭代器区间复制构造集合，按遍历顺序插入并去重。 */
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

	/** @brief 从迭代器区间移动候选元素构造集合，按遍历顺序插入并去重。 */
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

	/** @brief 复制可迭代对象中的元素构造集合。 */
	template<typename T, IteratableU<T> Obj>
	def set(const Obj& elems) { return set<T>(elems.begin(), elems.end()); }

	/** @brief 以移动方式传入可迭代对象中的候选元素，按遍历顺序构造集合。 */
	template<typename T, IteratableU<T> Obj>
	def set(Obj&& elems) { return set<T>(std::move(elems.begin()), std::move(elems.end())); }
}
#endif
