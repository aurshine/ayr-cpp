#ifndef AYR_AIR_TABLE_HPP
#define AYR_AIR_TABLE_HPP

#include "DynArray.hpp"

namespace ayr
{
	/** @brief 以 2 的幂作为容量、以 3/4 作为负载阈值的哈希表容量策略。 */
	class Pow2Policy
	{
		using self = Pow2Policy;

		// 当前容量, 2^n
		c_size capacity_;
	public:
		/** @brief 使用最小容量初始化策略。 */
		constexpr Pow2Policy() : capacity_(min_capacity()) {}

		/** @brief 复制容量配置。 */
		constexpr Pow2Policy(const self& other) : capacity_(other.capacity_) {}

		/** @brief 复制容量配置，支持自赋值。 */
		constexpr self& operator=(const self& other)
		{
			if (this == &other) return *this;

			capacity_ = other.capacity_;
			return *this;
		}

		/** @brief 返回当前槽位容量。 */
		constexpr c_size capacity() const { return capacity_; }

		/** @brief 返回最小槽位容量。 */
		constexpr c_size min_capacity() const { return 16; }

		/** @brief 返回策略允许的容量上限。 */
		constexpr c_size max_capacity() const { return 1ll << 62; }

		/** @brief 返回触发扩容的元素数量，即当前容量的 3/4。 */
		constexpr c_size load_threshold() const { return capacity() - (capacity() >> 2); }

		/** @brief 返回用于环形索引的容量掩码。 */
		constexpr c_size mask() const { return capacity() - 1; }

		/** @brief 将容量减半并返回新容量；调用方负责保证容量仍在有效范围内。 */
		constexpr c_size shrink_capacity() { return capacity_ >>= 1; }

		/** @brief 将容量加倍并返回新容量；调用方负责保证容量仍在有效范围内。 */
		constexpr c_size expand_capacity() { return capacity_ <<= 1; }

		/** @brief 使用哈希值的低位计算初始槽位索引。 */
		constexpr c_size hash2index(hash_t hashv) const { return hashv & mask(); }

		/** @brief 返回下一个槽位索引，到达表尾时回绕到表头。 */
		constexpr c_size next_index(c_size index) const { return (index + 1) & mask(); }

		/** @brief 恢复最小容量并返回该容量。 */
		constexpr c_size reset() { return capacity_ = min_capacity(); }

		/**
		 * @brief 按预期元素数量选择容量，使负载阈值严格大于 n。
		 * @param n 预期存储的元素数量。
		 * @return 调整后的槽位容量，至少为 min_capacity()。
		 * @note n 必须非负且小于最大容量的负载阈值，非法输入不改变当前容量。
		 */
		constexpr c_size adapt_at_least(c_size n)
		{
			const c_size max_elements = max_capacity() - (max_capacity() >> 2);
			if (n < 0 || n >= max_elements)
				RuntimeError("n is outside the supported element count range!");

			// 3 * capacity / 4 > n 等价于 capacity > 4 * n / 3。
			// 先验证上限，保证加法和 bit_ceil 的结果均可由 64 位整数表示。
			const auto count = static_cast<uint64_t>(n);
			const auto required = count + count / 3 + 1;
			const auto capacity = std::bit_ceil(required);
			return capacity_ = static_cast<c_size>(std::max(capacity, static_cast<uint64_t>(min_capacity())));
		}
	};

	/**
	 * @brief Table 表项需要提供的键、值、哈希及移动操作接口。
	 * @details 默认构造的表项可用于空槽位；键值访问仅在表项有效时使用。
	 * 相等的键必须具有相同的哈希，查询哈希必须与表项保存的哈希一致。
	 */
	template<typename KV>
	concept ItemConcept =
		std::default_initializable<KV> &&
		std::movable<KV> &&
		requires(KV & kv, const KV & ckv, const typename KV::Key & key)
	{
		typename KV::Key;
		typename KV::Val;

		// 比较 Key
		{ ckv.key_equal(key) } -> std::same_as<bool>;

		// 获取哈希值
		{ ckv.hashv() } -> std::same_as<hash_t>;

		// 获取 Key
		{ ckv.key() } -> std::same_as<const typename KV::Key&>;

		// 获取 Value
		{ kv.value() } -> std::same_as<typename KV::Val&>;
		{ ckv.value() } -> std::same_as<const typename KV::Val&>;
	};

	/**
	 * @brief 在业务表项上附加 Robin Hood 探测距离的槽位。
	 * @details Item 负责键值及其资源语义；dist 为 EMPTY_DIST 时表示空槽位。
	 */
	template<ItemConcept Item>
	class RobinItem: public Item
	{
		using super = Item;

		using self = RobinItem<super>;
	public:
		using Dist_t = int32_t;

		/** @brief 空槽位的探测距离标记。 */
		constexpr static Dist_t EMPTY_DIST = -1;

		// 从初始哈希槽位到当前槽位的环形探测距离。
		Dist_t dist = EMPTY_DIST;

		/** @brief 默认构造业务表项，并将槽位标记为空。 */
		constexpr RobinItem() : super() {}

		/** @brief 移动业务表项及探测距离，并将来源槽位标记为空。 */
		constexpr RobinItem(self&& other) : super(std::move(other)), dist(other.dist)
		{
			other.dist = EMPTY_DIST;
		}

		/** @brief 销毁槽位及其 Item 基类。 */
		constexpr ~RobinItem() { dist = EMPTY_DIST; }

		/** @brief 按成员复制槽位；是否可用取决于 Item 的拷贝赋值能力。 */
		constexpr self& operator=(const self& other) = default;

		/** @brief 按成员移动业务表项并赋值距离，不重置来源槽位的距离。 */
		constexpr self& operator=(self&& other) = default;

		/** @brief 判断槽位是否已被占用。 */
		constexpr bool used() const { return dist != EMPTY_DIST; }

		/**
		 * @brief 先比较缓存哈希，再比较键是否相等。
		 * @pre 当前 Item 必须有效；本函数不检查槽位是否被占用。
		 */
		constexpr bool check_kv_item(const typename super::Key& key, hash_t hashv) const
		{
			return super::hashv() == hashv && super::key_equal(key);
		}

		/** @brief 仅将槽位标记为空，不重置或销毁 Item。 */
		constexpr void set_unused() { dist = EMPTY_DIST; }

		/**
		 * @brief 移入业务表项，并设置当前槽位的探测距离。
		 * @param dist 新表项到当前槽位的探测距离。
		 * @param kv_item 要移入的业务表项。
		 */
		constexpr void set_kv_item(Dist_t dist, super&& kv_item) 
		{ 
			super::operator=(std::move(kv_item)); 
			this->dist = dist;
		}

		/**
		 * @brief 交换业务表项及探测距离，供 Robin Hood 插入继续搬移被挤出的表项。
		 * @param dist 待插入表项的距离；交换后保存原槽位表项的距离。
		 * @param kv_item 待插入表项；交换后保存原槽位的业务表项。
		 */
		constexpr void swap_kv_item(Dist_t& dist, super& kv_item)
		{
			std::swap(this->dist, dist);
			std::swap(static_cast<super&>(*this), kv_item);
		}

		/** @brief 输出槽位占用状态，或已占用槽位的距离和值。 */
		void __repr__(Buffer& buffer) const
		{
			if (!used()) 
				buffer << "[unused]";
			else
				buffer << "[dist:" << dist << " value:" << this->value() << "]";
		}
	};

	/**
	 * @brief 使用开放寻址和 Robin Hood 探测的哈希表，仅支持移动。
	 * @details 缓存哈希并通过键相等判断区分哈希碰撞，负载达到 3/4 时扩容。
	 * 表内保存 Item 对象；DictItem 和 SetItem 引用的链表节点由上层容器管理。
	 */
	template<ItemConcept Item>
	class Table
	{
		using self = Table<Item>;

		using Dist_t = typename RobinItem<Item>::Dist_t;

		using Key = typename Item::Key;
	public:
		/** @brief 一次探测的候选槽位、探测距离和键命中状态。 */
		struct ProbeResult
		{
			// 命中时为目标槽位，未命中时为插入起点。
			c_size index;

			// 查询键从初始哈希槽位到 index 的探测距离。
			Dist_t dist;

			// 哈希与键均匹配时为 true，无需再次比较键。
			bool found;
		};

		using RobinItem_t = RobinItem<Item>;

		// 连续槽位数组，扩容时重新分配。
		RobinItem_t* items_;

		// 槽位容量和环形索引策略。
		Pow2Policy policy_;

		// 已占用槽位的数量。
		c_size size_;

		/**
		 * @brief 根据预期元素数量分配并默认构造槽位。
		 * @param capacity 预期元素数量；0 表示使用策略的最小槽位容量。
		 */
		Table(c_size capacity = 0) : policy_(), items_(nullptr), size_(0)
		{
			items_ = ayr_alloc<RobinItem_t>(policy_.adapt_at_least(capacity));
			for (c_size i = 0, n = this->capacity(); i < n; ++i)
				ayr_construct(items_ + i);
		}

		/** @brief 接管槽位数组及其容量和计数，来源数组指针置空、计数归零。 */
		Table(self&& other) noexcept : policy_(std::move(other.policy_)), items_(other.items_), size_(other.size_)
		{
			other.items_ = nullptr;
			other.size_ = 0;
		}

		/** @brief 销毁全部槽位并释放数组存储。 */
		~Table() { ayr_desloc(items_, policy_.capacity()); }

		/** @brief 释放当前槽位数组并接管来源存储，支持自移动赋值。 */
		self& operator=(self&& other) noexcept
		{
			if (this == &other) return *this;

			ayr_destroy(this);
			return *ayr_construct(this, std::move(other));
		}

		/** @brief 返回已占用槽位的数量。 */
		c_size size() const { return size_; }

		/** @brief 判断是否没有已占用的槽位。 */
		bool empty() const { return size_ == 0; }

		/** @brief 返回槽位数组的容量。 */
		c_size capacity() const { return policy_.capacity(); }

		/**
		 * @brief 探测键，返回命中状态及目标槽位或插入起点。
		 * @details 遇到空槽位或探测距离小于当前查找距离的表项时，说明键不存在；
		 * 仅哈希相等不足以命中，键也必须相等。查找过程不修改表。
		 * @param key 待查找的键。
		 * @param hashv 待查找键的哈希值。
		 * @return 包含槽位索引、探测距离和 found 标记的探测结果。
		 * @note found 为 false 时可在 index、dist 指示的位置开始插入；表修改后需重新探测。
		 */
		ProbeResult probe(const Key& key, hash_t hashv) const
		{
			c_size i = policy_.hash2index(hashv);
			Dist_t move_dist = 0;
			while (true)
			{
				const auto& item = items_[i];
				// 空槽位的距离为 -1，会先短路返回，避免访问无效的业务表项。
				// 当前查找距离更大时，后续槽位也不会包含该键，可在此开始插入。
				if (move_dist > item.dist)
					return { i, move_dist, false };

				if (item.check_kv_item(key, hashv))
					return { i, move_dist, true };

				++move_dist;
				i = policy_.next_index(i);
			};
		}

		/** @brief 根据键及其哈希判断目标表项是否存在。 */
		bool contains(const Key& key, hash_t hashv) const
		{
			return probe(key, hashv).found;
		}

		/**
		 * @brief 从候选槽位开始插入新表项，必要时搬移表项并扩容。
		 * @param index probe() 未命中时返回的插入起点。
		 * @param move_dist 新表项到插入起点的探测距离。
		 * @param kv_item 要移入的业务表项。
		 * @pre 调用方已确认该键不存在，且 index、move_dist 与该键的哈希一致。
		 */
		void insert_value_on_index(c_size index, Dist_t move_dist, Item&& kv_item)
		{
			while (items_[index].used())
			{
				if (move_dist > items_[index].dist)
					items_[index].swap_kv_item(move_dist, kv_item);
				index = policy_.next_index(index);
				++move_dist;
			}

			items_[index].set_kv_item(move_dist, std::move(kv_item));
			++size_;

			try_expand();
		}

		/**
		 * @brief 删除指定槽位的表项，并将后续冲突表项向前回移。
		 * @param index 要删除的有效槽位索引；本函数不执行键匹配。
		 * @return 槽位已占用并完成删除时为 true，空槽位返回 false。
		 * @note 删除前若需保留表项中的节点指针，应先保存；回移可能覆盖原槽位。
		 * Dict/Set 在本函数返回后负责释放保存的链表节点。
		 */
		bool pop_value_on_index(c_size index)
		{
			if (!items_[index].used()) return false;

			// i 是待填补的位置，j 是下一候选回移槽位；表尾按容量掩码回绕。
			c_size i = index, j = policy_.next_index(i);

			// 仅回移偏离初始桶的表项；遇到空槽位或距离为 0 的表项即停止。
			while (items_[j].dist > 0)
			{
				std::swap(items_[i], items_[j]);
				items_[i].dist -= 1;

				i = j;
				j = policy_.next_index(j);
			}

			items_[i].set_unused();
			--size_;
			return true;
		}

		/** @brief 销毁并释放旧槽位数组，重新分配最小容量的空表。 */
		void clear()
		{
			const c_size old_capacity = capacity();
			ayr_desloc(items_, old_capacity);
			policy_.reset();
			items_ = ayr_alloc<RobinItem_t>(capacity());
			for (c_size i = 0, n = capacity(); i < n; ++i)
				ayr_construct(items_ + i);
			size_ = 0;
		}

		/**
		 * @brief 元素数量达到负载阈值时，将容量加倍并重新分布全部表项。
		 * @details 使用 Item 中缓存的哈希计算新位置，不重新计算键的哈希。
		 */
		void try_expand()
		{
			if (size_ < policy_.load_threshold()) return;

			c_size n = policy_.capacity();
			// 构造函数按预期元素数适配容量；传入当前容量会分配两倍槽位。
			self new_table(policy_.capacity());
			new_table.size_ = size_;

			for (c_size i = 0; i < n; ++i)
				if (items_[i].used())
					new_table.insert_when_rehash(static_cast<Item&&>(items_[i]));

			*this = std::move(new_table);
		}

		/**
		 * @brief 扩容重排时，使用缓存哈希移入一个表项。
		 * @param kv_item 要移入的业务表项。
		 * @pre 目标表有空槽位，且该键尚未插入。
		 * @note 不检查重复键、不增加 size_、不触发扩容；计数由调用方统一设置。
		 */
		void insert_when_rehash(Item&& kv_item)
		{
			Dist_t move_dist = 0;
			c_size j = policy_.hash2index(kv_item.hashv());

			while (true)
			{
				auto& item = items_[j];
				if (!item.used())
				{
					item.set_kv_item(move_dist, std::move(kv_item));
					return;
				}

				if (move_dist > item.dist)
					item.swap_kv_item(move_dist, kv_item);

				j = policy_.next_index(j);
				++move_dist;
			}
		}

		/** @brief 按槽位顺序输出整张表，包括空槽位。 */
		void __repr__(Buffer& buffer) const
		{
			for (c_size i = 0, n = capacity(); i < n; ++i)
				buffer << items_[i];
		}
	};
}
#endif // AYR_AIR_TABLE_HPP
