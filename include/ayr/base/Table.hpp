#ifndef AYR_BASE_TABLE_HPP
#define AYR_BASE_TABLE_HPP

#include <array>

#include "bunit.hpp"
#include "IteratorInfo.hpp"
#include "raise_error.hpp"
#include "../DynArray.hpp"

namespace ayr
{
	class Pow2Policy : Object<Pow2Policy>
	{
		using self = Pow2Policy;

		using super = Object<self>;

		// 当前容量, 2^n
		c_size capacity_;
	public:
		constexpr Pow2Policy() : capacity_(min_capacity()) {}

		constexpr Pow2Policy(const self& other) : capacity_(other.capacity_) {}

		constexpr self& operator=(const self& other)
		{
			if (this == &other) return *this;

			capacity_ = other.capacity_;
			return *this;
		}

		// 当前策略的容量
		constexpr c_size capacity() const { return capacity_; }

		// 最小容量
		constexpr c_size min_capacity() const { return 16; }

		// 最大容量
		constexpr c_size max_capacity() const { return 1ll << 62; }

		// 负载阈值, 负载因子为 3 / 4
		constexpr c_size load_threshold() { return (capacity() >> 2) * 3; }

		// 掩码
		constexpr c_size mask() const { return capacity() - 1; }

		// 缩小容量，并返回缩小后的容量
		constexpr c_size shrink_capacity() { return capacity_ >>= 1; }

		// 扩大容量，并返回扩大后的容量
		constexpr c_size expand_capacity() { return capacity_ <<= 1; }

		// hash值转化为索引
		constexpr c_size hash2index(hash_t hashv) const { return hashv & mask(); }

		// 下一个索引
		constexpr c_size next_index(c_size index) const { return (index + 1) & mask(); }

		// 调整到初始值
		constexpr c_size reset() { return capacity_ = min_capacity(); }

		/*
		* @brief 容量调整到最小满足负载阈值的容量
		*
		* @param n 期望的容量
		*
		* @return 调整后的容量
		*/
		c_size adapt_at_least(c_size n)
		{
			if (n > max_capacity())
				RuntimeError("n is too large!");

			/*
			* 假设 n 的二进制形式为 b[n] b[n - 1] .... b[0]
			*
			* 其中b[n] 为最高位
			*
			* 需要计算值 res，使得 res * 3 / 4 > n
			*
			* res的二进制形式为 r[k + 1] r[k] r[k - 1] ... r[0]
			*
			* r[k] 为最高位为1，其余位为0
			*
			* r[k + 1] 为附加位，也为0
			*
			* res * 3 可表示为 res * 2 + res
			*
			* 即让res[k + 1] 为 1
			*
			* 再 / 4，r[k - 1] = 1, r[k - 2] = 1，其余为为0
			*
			* 此时最高位为r[k - 1]为1，与b[n]对齐比较
			*
			* 当b[n] = 1, b[n - 1] = 0，其余位为任意值，则k - 1 = n
			*
			* 当b[n] = 1，b[n - 1] = 1，其余位为任意值，则 k - 1 = n + 1
			*
			* 即当b最高位为 10, res = 1 << (n + 1);
			*
			* 当b最高位为 11，res = 1 << (n + 2);
			*/
			c_size h_index = highbit_index(n);
			c_size res = 0;
			if (h_index <= 0 || (((n >> (h_index - 1)) & 1) == 0))
				res = 1 << (h_index + 1);
			else
				res = 1 << (h_index + 2);

			// 溢出
			if (res <= 0)
				RuntimeError("n is too large!");
			if (res < min_capacity())
				res = min_capacity();
			return capacity_ = res;
		}
	};

	template<typename T>
	class RobinItem : Object<RobinItem<T>>
	{
		using self = RobinItem<T>;

		using super = Object<self>;

		std::aligned_storage_t<sizeof(T), alignof(T)> value_;
	public:
		using Dist_t = int32_t;

		constexpr static Dist_t EMPITY_DIST = -1;

		// 元素的移动距离
		Dist_t dist;

		// 元素的hash值
		hash_t hashv;

		constexpr RobinItem() : hashv(0), dist(-1) {}

		RobinItem(const self& other) : RobinItem()
		{
			if (other.used())
				set_empty_value(other.hashv, other.dist, other.value());
		}

		RobinItem(self&& other) : RobinItem()
		{
			if (other.used())
			{
				set_empty_value(other.hashv, other.dist, std::move(other.value()));
				other.set_unused();
			}
		}

		~RobinItem() { if (used()) { destroy_value(); } }

		self& operator=(const self& other)
		{
			if (this == &other) return *this;

			ayr_destroy(this);
			return *ayr_construct(this, other);
		}

		self& operator=(self&& other)
		{
			if (this == &other) return *this;

			ayr_destroy(this);
			return *ayr_construct(this, std::move(other));
		}

		// 元素是否被使用
		constexpr bool used() const { return dist != EMPITY_DIST; }

		// value必须有效，将元素设置为不使用
		void set_unused() { destroy_value(); dist = EMPITY_DIST; }

		// 不检查value的有效性,value必须有效
		T& value() { return reinterpret_cast<T&>(value_); }

		// 不检查value的有效性,value必须有效
		const T& value() const { return reinterpret_cast<const T&>(value_); }

		// 原来不存在value, 构造一个新的value
		template<typename... Args>
		void set_empty_value(const hash_t& hashv, const Dist_t& dist, Args&& ... args)
		{
			ayr_construct(&value(), std::forward<Args>(args)...);
			this->hashv = hashv;
			this->dist = dist;
		}

		// 原来存在value, 设置一个新的value
		template<typename ... Args>
		void set_new_value(Args&&... args)
		{
			destroy_value();
			ayr_construct(&value(), std::forward<Args>(args)...);
		}

		// 原来存在value, 交换value, hashv, dist
		void swap_elements(hash_t& hashv, Dist_t& dist, T& value)
		{
			ayr::swap(this->value(), value);
			swap(this->hashv, hashv);
			swap(this->dist, dist);
		}

		// 销毁value，value必须有效
		void destroy_value()
		{
			ayr_destroy(&value());
			hashv = 0;
			dist = 0;
		}

		void __repr__(Buffer& buffer) const
		{
			if (!used()) buffer << "[unused]";
			else
				buffer << "[hashv:" << hashv << " dist:" << dist << " value:" << value() << "]";
		}

		void __swap__(self& other)
		{
			if (used() && other.used())
				swap_elements(other.hashv, other.dist, other.value());
			else
				RuntimeError("some element is unused");
		}
	};

	template<typename T>
	class Table : public Object<Table<T>>
	{
		using self = Table<T>;

		using super = Object<self>;

		using Dist_t = typename RobinItem<T>::Dist_t;

	public:
		RobinItem<T>* items_;

		Pow2Policy policy_;

		c_size size_;

		using RobinItem_t = RobinItem<T>;

		using Value_t = T;

		// capacity = 0时选择policy的最小容量方案
		Table(c_size capacity = 0) : policy_(), items_(nullptr), size_(0)
		{
			items_ = ayr_alloc<RobinItem_t>(policy_.adapt_at_least(capacity));
			for (c_size i = 0, n = this->capacity(); i < n; ++i)
				ayr_construct(items_ + i);
		}

		Table(const self& other) : policy_(other.policy_), items_(nullptr), size_(other.size_)
		{
			items_ = ayr_alloc<RobinItem_t>(capacity());
			for (c_size i = 0, n = capacity(); i < n; ++i)
				ayr_construct(items_ + i, other.items_[i]);
		}

		Table(self&& other) noexcept : policy_(std::move(other.policy_)), items_(other.items_), size_(other.size_)
		{
			other.items_ = nullptr;
			other.size_ = 0;
		}

		~Table() { ayr_desloc(items_, policy_.capacity()); }

		self& operator=(const self& other)
		{
			if (this == &other) return *this;

			ayr_destroy(this);
			return *ayr_construct(this, other);
		}

		self& operator=(self&& other) noexcept
		{
			if (this == &other) return *this;

			ayr_destroy(this);
			return *ayr_construct(this, std::move(other));
		}

		c_size size() const { return size_; }

		c_size capacity() const { return policy_.capacity(); }

		/*
		* @brief 找到hashv最适配的元素索引
		*
		* @detail
		* - 未使用表示当前table没有该元素，该位置可以插入元素
		* - hashv不相等表示当前table没有该元素，该位置插入元素需要整体后移
		* - hashv相等表示找到了对应的元素，该位置可以更新元素
		*
		* @param hashv 元素的hash值
		*
		* @return 索引和移动距离
		*/
		std::pair<c_size, Dist_t> try_get(const hash_t& hashv) const
		{
			c_size j = policy_.hash2index(hashv);
			c_size move_dist = 0;
			while (true)
			{
				// 元素为空时，dist == -1，move_dist > items_[j]一定成立
				// 当前的移动距离大于items_[j]元素的移动距离，可以替换items_[j]元素
				// 或者当前元素的hash值等于hashv
				if (move_dist > items_[j].dist || items_[j].hashv == hashv)
					return { j, move_dist };

				++move_dist;
				j = policy_.next_index(j);
			};
		}

		// table是否包含hashv对应的元素
		bool contains(hash_t hashv) const
		{
			auto [index, move_dist] = try_get(hashv);
			return items_[index].used() && items_[index].hashv == hashv;
		}

		/*
		* @brief 存入value到hashv对应的元素
		*
		* @detail
		* - 如果没有找到，则插入到第一个空闲的位置
		* - 如果找到，则更新对应的元素
		*
		* @param hashv 元素的hash值
		*
		* @param args 元素的构造参数
		*/
		template<typename ... Args>
		void insert(const hash_t& hashv, Args&&... args)
		{
			auto [index, move_dist] = try_get(hashv);

			// 找到的是对应自己的元素
			if (items_[index].used() && items_[index].hashv == hashv)
				items_[index].set_new_value(std::forward<Args>(args)...);
			else
				insert_value_on_index(index, hashv, move_dist, std::forward<Args>(args)...);
		}

		/*
		* @brief 从index开始插入或更新value
		*
		* @param index 插入位置
		*
		* @param hashv 元素的hash值
		*
		* @param move_dist 元素移动的距离
		*
		* @param value 元素的值
		*/
		void insert_value_on_index(c_size index, hash_t hashv, Dist_t move_dist, Value_t value)
		{
			while (items_[index].used())
			{
				if (move_dist > items_[index].dist)
					items_[index].swap_elements(hashv, move_dist, value);
				index = policy_.next_index(index);
				++move_dist;
			}

			items_[index].set_empty_value(hashv, move_dist, std::move(value));
			++size_;

			try_expand();
		}

		/*
		* @brief 从index开始插入或更新value
		*
		* @param index 插入位置
		*
		* @param hashv 元素的hash值
		*
		* @param move_dist 元素移动的距离
		*
		* @param args 构造元素的值
		*/
		template<typename ... Args>
		void insert_value_on_index(c_size index, hash_t hashv, Dist_t move_dist, Args&&... args)
		{
			insert_value_on_index(index, hashv, move_dist, Value_t{ std::forward<Args>(args)... });
		}

		/*
		* @brief 删除hashv对应的元素
		*
		* @param hashv 元素的hash值
		*
		* @return 是否删除成功
		*/
		bool pop(const hash_t& hashv)
		{
			auto [index, move_dist] = try_get(hashv);
			return pop_value_on_index(index, hashv, move_dist);
		}

		/*
		* @brief 从index开始找到hashv对应的元素并删除
		*
		* @param index 元素的索引
		*
		* @param hashv 元素的hash值
		*
		* @param move_dist 元素移动的距离
		*
		* @return 是否删除成功
		*/
		bool pop_value_on_index(c_size index, hash_t hashv, Dist_t move_dist)
		{
			// 没找到对应元素
			if (!items_[index].used() || items_[index].hashv != hashv)
				return false;


			// 当前下标和下一个下标，下一个下标的值 -> 当前下标的值
			c_size i = index, j = policy_.next_index(i);

			// items_[j] 可以向前移动获得更快的查询速度
			while (items_[j].dist > 0)
			{
				items_[i].swap_elements(items_[j].hashv, items_[j].dist, items_[j].value());
				items_[i].dist -= 1;

				i = j;
				j = policy_.next_index(j);
			}

			items_[i].set_unused();
			--size_;
			return true;
		}

		// 清空表并且释放内存
		void clear()
		{
			policy_.reset();
			ayr_desloc(items_, policy_.capacity());
			items_ = ayr_alloc<RobinItem_t>(capacity());
			for (c_size i = 0, n = capacity(); i < n; ++i)
				ayr_construct(items_ + i);
			size_ = 0;
		}

		/*
		* @brief 尝试扩容
		*
		* @detail
		* - 当元素个数小于阈值时，不扩容
		* - 当元素个数大于等于阈值时，扩容到原来的两倍
		*/
		void try_expand()
		{
			if (size_ < policy_.load_threshold()) return;

			c_size n = policy_.capacity();
			// 根据policy的at_least策略，实际容量是传入数值的两倍
			self new_table(policy_.capacity());
			new_table.size_ = size_;

			for (c_size i = 0; i < n; ++i)
				if (items_[i].used())
					new_table.insert_value_on_rehash(items_[i].hashv, std::move(items_[i].value()));

			*this = std::move(new_table);
		}

		/*
		* @brief rehash时插入元素，假定所有插入元素都不存在
		*
		* @param hashv 元素的hash值
		*
		* @param value 元素的值
		*/
		void insert_value_on_rehash(hash_t hashv, Value_t&& value)
		{
			Dist_t move_dist = 0;
			c_size j = policy_.hash2index(hashv);

			while (true)
			{
				if (!items_[j].used())
				{
					items_[j].set_empty_value(hashv, move_dist, std::move(value));
					return;
				}

				if (move_dist > items_[j].dist)
					items_[j].swap_elements(hashv, move_dist, value);

				j = policy_.next_index(j);
				++move_dist;
			}
		}

		void __repr__(Buffer& buffer) const
		{
			for (c_size i = 0, n = capacity(); i < n; ++i)
				buffer << items_[i];
		}

		void __swap__(self& other)
		{
			swap(items_, other.items_);
			policy_.__swap__(other.policy_);
			swap(size_, other.size_);
		}
	};
}
#endif // AYR_BASE_TABLE_HPP