#ifndef AYR_BASE_TABLE_HPP
#define AYR_BASE_TABLE_HPP

#include <array>

#include "bunit.hpp"
#include "hash.hpp"
#include "IteratorInfo.hpp"
#include "raise_error.hpp"

#include "../DynArray.hpp"

namespace ayr
{
	class PowerOfTwoPolicy : Object<PowerOfTwoPolicy>
	{
		using self = PowerOfTwoPolicy;

		using super = Object<self>;

		c_size capacity_;
	public:
		c_size load_threshold;

		constexpr static double load_factor = 0.75;

		PowerOfTwoPolicy() : capacity_(min_capacity()), load_threshold(min_capacity()* load_factor) {}

		PowerOfTwoPolicy(const self& other) : capacity_(other.capacity_), load_threshold(other.load_threshold) {}

		self& operator=(const self& other)
		{
			if (this == &other) return *this;

			capacity_ = other.capacity_;
			load_threshold = other.load_threshold;
			return *this;
		}

		// 当前策略的容量
		c_size capacity() const { return capacity_; }

		c_size prev()
		{
			capacity_ >>= 1;
			update_load_threshold();
			return capacity_;
		}

		// 尝试下一个策略
		c_size next()
		{
			capacity_ <<= 1;
			update_load_threshold();
			return capacity_;
		}

		// 调整到比n大的最小容量策略
		c_size adapt(c_size n)
		{
			if (n > max_capacity()) RuntimeError("capacity is too large!");

			if (capacity() < n)
			{
				while (next() < n);
			}
			else
			{
				while (capacity() > min_capacity() && (capacity() >> 1) >= n) prev();
			}
			return capacity();
		}

		// hash值转化为索引
		c_size hash2index(hash_t hashv) const { return hashv & (capacity_ - 1); }

		// 下一个索引
		c_size next_index(c_size index) const { return (index + 1) % capacity_; }

		// 调整到初始值
		c_size reset()
		{
			capacity_ = min_capacity();
			update_load_threshold();
			return capacity_;
		}

		// 最小容量
		constexpr c_size min_capacity() const { return 16; }

		// 最大容量
		constexpr c_size max_capacity() const { return 1ll << 62; }

	private:
		void update_load_threshold() { load_threshold = capacity() * load_factor; }
	};

	class PrimePolicy : Object<PrimePolicy>
	{
		using self = PrimePolicy;

		using super = Object<self>;

		int i = 0;
	public:
		constexpr static double load_factor = 0.75;

		c_size load_threshold;

		constexpr static std::array<c_size, 49> PRIMES = {
			13ll,
			23ll,
			37u,
			53u,
			67u,
			79u,
			97u,
			131u,
			193u,
			257u,
			389u,
			521u,
			769u,
			1031u,
			1543u,
			2053u,
			3079u,
			6151u,
			12289u,
			24593u,
			49157u,
			98317ul,
			196613ul,
			393241ul,
			786433ul,
			1572869ul,
			3145739ul,
			6291469ul,
			12582917ul,
			25165843ul,
			50331653ul,
			100663319ul,
			201326611ul,
			402653189ul,
			805306457ul,
			1610612741ul,
			3221225473ul,
			4294967291ul,
			6442450939ull,
			12884901893ull,
			25769803751ull,
			51539607551ull,
			103079215111ull,
			206158430209ull,
			412316860441ull,
			824633720831ull,
			1649267441651ull,
			3298534883309ull,
			6597069766657ull
		};

		PrimePolicy() : i(0), load_threshold(PRIMES[0] * load_factor) {}

		PrimePolicy(const self& other) : i(other.i), load_threshold(other.load_threshold) {}

		self& operator=(const self& other)
		{
			if (this == &other) return *this;

			i = other.i;
			load_threshold = other.load_threshold;
			return *this;
		}

		// 当前策略的容量
		c_size capacity() const { return PRIMES[i]; }

		// 尝试上一个策略
		c_size prev()
		{
			--i;
			update_load_threshold();
			return capacity();
		}

		// 尝试下一个策略
		c_size next()
		{
			++i;
			update_load_threshold();
			return capacity();
		}

		// 调整到比n大的最小容量策略
		c_size adapt(c_size n)
		{
			if (n > max_capacity()) RuntimeError("capacity is too large!");

			if (capacity() < n)
			{
				while (next() < n);
			}
			else
			{
				while (i > 0 && PRIMES[i - 1] >= n) prev();
			}
			return capacity();
		}

		// hash值转化为索引
		c_size hash2index(hash_t hashv) const { return hashv % PRIMES[i]; }

		// 下一个索引
		c_size next_index(c_size index) const { return (index + 1) % PRIMES[i]; }

		// 调整到初始值
		c_size reset()
		{
			i = 0;
			update_load_threshold();
			return capacity();
		}

		// 最小容量
		constexpr c_size min_capacity() const { return PRIMES.front(); }

		// 最大容量
		constexpr c_size max_capacity() const { return PRIMES.back(); }
	private:
		void update_load_threshold() { load_threshold = capacity() * load_factor; }
	};

	template<typename T>
	class TableItem : Object<TableItem<T>>
	{
		using self = TableItem<T>;

		using super = Object<self>;

		union {
			T value_;
			uint8_t dummy_;
		};
	public:
		using Dist_t = int32_t;

		constexpr static Dist_t EMPITY_DIST = -1;

		Dist_t dist;

		hash_t hashv;

		TableItem() : dummy_(0), hashv(0), dist(-1) {}

		TableItem(const self& other) : TableItem()
		{
			if (other.used())
				set_empty_value(other.hashv, other.dist, other.value());
		}

		TableItem(self&& other) : TableItem()
		{
			if (other.used())
			{
				set_empty_value(other.hashv, other.dist, std::move(other.value()));
				other.set_unused();
			}
		}

		~TableItem() { if (used()) { destroy_value(); } }

		self& operator=(const self& other)
		{
			if (this == &other) return *this;

			if (used()) destroy_value();
			if (other.used())
				set_empty_value(other.hashv, other.dist, other.value());
			return *this;
		}

		self& operator=(self&& other)
		{
			if (this == &other) return *this;

			if (used()) destroy_value();
			if (other.used())
			{
				set_empty_value(other.hashv, other.dist, std::move(other.value()));
				set_unused();
			}
			return *this;
		}

		bool used() const { return dist >= 0; }

		// value必须有效
		void set_unused() { destroy_value(); dist = -1; }

		// 不检查value的有效性,value必须有效
		T& value() { return value_; }

		// 不检查value的有效性,value必须有效
		const T& value() const { return value_; }

		// 原来不存在value, 构造一个新的value
		template<typename... Args>
		void set_empty_value(const hash_t& hashv, const Dist_t& dist, Args&& ... args)
		{
			ayr_construct(&value_, std::forward<Args>(args)...);
			this->hashv = hashv;
			this->dist = dist;
		}

		// 构造一个新的value, value必须有效
		template<typename ... Args>
		void set_new_value(Args&&... args)
		{
			destroy_value();
			ayr_construct(&value_, std::forward<Args>(args)...);
		}

		// 交换value, hashv, dist, value必须有效
		void swap_elements(T& value, hash_t& hashv, Dist_t& dist)
		{
			ayr::swap(value_, value);
			swap(this->hashv, hashv);
			swap(this->dist, dist);
		}

		CString __str__() const
		{
			if (!used()) return "[unused]";
			return std::format("[hashv:{} dist:{} value:{}]", hashv, dist, value());
		}

		void __swap__(self& other)
		{
			swap(hashv, other.hashv);
			swap(dist, other.dist);

			uint8_t tmp[sizeof(T)];
			std::memcpy(tmp, &value_, sizeof(T));
			std::memcpy(&value_, &other.value_, sizeof(T));
			std::memcpy(&other.value_, tmp, sizeof(T));
		}

		// 销毁value，value必须有效
		void destroy_value() { ayr_destroy(&value_); }
	};

	template<typename T>
	class Table : public Object<Table<T>>
	{
		using self = Table<T>;

		using super = Object<self>;

		using Dist_t = typename TableItem<T>::Dist_t;

	public:
		TableItem<T>* items_;

		PrimePolicy policy_;

		c_size size_;

		using TableItem_t = TableItem<T>;

		using Value_t = T;

		Table() : policy_(), items_(nullptr), size_(0)
		{
			items_ = ayr_alloc<TableItem_t>(capacity());
			for (c_size i = 0, n = capacity(); i < n; ++i)
				ayr_construct(items_ + i);
		}

		Table(c_size capacity) : policy_(), items_(nullptr), size_(0)
		{
			items_ = ayr_alloc<TableItem_t>(policy_.adapt(capacity));
			for (c_size i = 0, n = this->capacity(); i < n; ++i)
				ayr_construct(items_ + i);
		}

		Table(const self& other) : policy_(other.policy_), items_(nullptr), size_(other.size_)
		{
			items_ = ayr_alloc<TableItem_t>(capacity());
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

		// 找到hashv最适配的元素索引
		// 返回索引和移动距离
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

		// 存入value到hashv对应的元素
		// 如果没有找到，则插入到第一个空闲的位置
		// 如果找到，则更新对应的元素
		// 返回插入的元素和表示是否新增元素的bool值
		template<typename ... Args>
		void insert(const hash_t& hashv, Args&&... args)
		{
			try_expand();
			auto [index, move_dist] = try_get(hashv);

			if (items_[index].used())
				if (items_[index].hashv == hashv)// 找到的是对应自己的元素
				{
					items_[index].set_new_value(std::forward<Args>(args)...);
					return;
				}
				else // 找到的是自己合适但是是别的元素
					insert_value_on_index(index, hashv, move_dist, std::forward<Args>(args)...);
			else // 找到的是空闲的元素
				items_[index].set_empty_value(hashv, move_dist, std::forward<Args>(args)...);
			++size_;
		}

		// 从index开始插入value
		// value已经移动的距离为move_dist
		// index的元素不应该为空
		template<typename ... Args>
		void insert_value_on_index(c_size index, hash_t hashv, Dist_t move_dist, Args&&... args)
		{
			Value_t value(std::forward<Args>(args)...);

			items_[index].swap_elements(value, hashv, move_dist);
			c_size j = policy_.next_index(index);
			++move_dist;

			while (items_[j].used())
			{
				if (move_dist > items_[j].dist)
					items_[j].swap_elements(value, hashv, move_dist);
				j = policy_.next_index(j);
				++move_dist;
			}
			items_[j].set_empty_value(hashv, move_dist, std::move(value));
		}

		// 尝试删除hashv对应的元素
		// 如果没有找到，则什么也不做，返回false
		// 如果找到，则删除对应的元素，返回true
		bool pop(const hash_t& hashv)
		{
			auto [index, move_dist] = try_get(hashv);
			return pop_value_on_index(index, hashv, move_dist);
		}

		bool pop_value_on_index(c_size index, hash_t hashv, Dist_t move_dist)
		{
			// 没找到对应元素
			if (!items_[index].used() || items_[index].hashv != hashv)
				return false;

			items_[index].set_unused();
			// 当前下标和下一个下标，下一个下标的值 -> 当前下标的值
			c_size i = index, j = policy_.next_index(i);
			while (true)
			{
				if (items_[j].dist <= 0) break;
				items_[i].set_empty_value(items_[j].hashv, items_[j].dist - 1, std::move(items_[j].value()));
				items_[j].dist = -1;

				i = j;
				j = policy_.next_index(j);
			}
			--size_;
			return true;
		}

		// 清空表并且释放内存
		void clear()
		{
			policy_.reset();
			ayr_desloc(items_, policy_.capacity());
			items_ = ayr_alloc<TableItem_t>(capacity());
			for (c_size i = 0, n = capacity(); i < n; ++i)
				ayr_construct(items_ + i);
			size_ = 0;
		}

		// 扩容
		void try_expand()
		{
			if (size_ < policy_.load_threshold) return;

			c_size n = policy_.capacity();
			self new_table(policy_.next());
			for (c_size i = 0; i < n; ++i)
				if (items_[i].used())
					new_table.insert_value_on_rehash(items_[i].hashv, std::move(items_[i].value()));

			ayr_delloc(items_);
			items_ = new_table.items_;
			new_table.items_ = nullptr;
		}

		CString __str__() const
		{
			DynArray<CString> strs;
			for (c_size i = 0, n = capacity(); i < n; ++i)
				strs.append(items_[i].__str__());
			return cstr("\n").join(strs);
		}

		void __swap__(self& other)
		{
			swap(items_, other.items_);
			policy_.__swap__(other.policy_);
			swap(size_, other.size_);
		}

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
					items_[j].swap_elements(value, hashv, move_dist);

				j = policy_.next_index(j);
				++move_dist;
			}
		}

		template<bool IsConst>
		class TableIterator : public IteratorInfo<
			TableIterator<IsConst>,
			add_const_t<IsConst, self>,
			std::bidirectional_iterator_tag,
			add_const_t<IsConst, Value_t>>
		{
			using ItInfo = IteratorInfo<TableIterator<IsConst>,
				add_const_t<IsConst, self>,
				std::bidirectional_iterator_tag,
				add_const_t<IsConst, Value_t>>;

			ItInfo::container_type* container_;

			c_size ith_;
		public:
			TableIterator() : container_(nullptr), ith_(0) {}

			TableIterator(ItInfo::container_type* container, c_size ith) : container_(container), ith_(ith) {}

			TableIterator(const typename ItInfo::iterator_type& other) : TableIterator(other.container_, other.ith_) {}

			typename ItInfo::iterator_type operator=(const typename ItInfo::iterator_type& other)
			{
				if (this == &other) return *this;
				container_ = other.container_;
				ith_ = other.ith_;
				return *this;
			}

			ItInfo::reference operator*() const { return container_->items_[ith_].value(); }

			ItInfo::pointer operator->() const { return &container_->items_[ith_].value(); }

			typename ItInfo::iterator_type& operator++()
			{
				++ith_;
				while (ith_ < container_->capacity() && !container_->items_[ith_].used()) ++ith_;
				return *this;
			}

			typename ItInfo::iterator_type operator++(int)
			{
				typename ItInfo::iterator_type tmp = *this;
				++*this;
				return tmp;
			}

			typename ItInfo::iterator_type& operator--()
			{
				--ith_;
				while (ith_ > 0 && !container_->items_[ith_].used()) --ith_;
				return *this;
			}

			typename ItInfo::iterator_type operator--(int)
			{
				typename ItInfo::iterator_type tmp = *this;
				--*this;
				return tmp;
			}

			bool __equals__(const typename ItInfo::iterator_type& other) const
			{
				return container_ == other.container_ && ith_ == other.ith_;
			}
		};

		using Iterator = TableIterator<false>;

		using ConstIterator = TableIterator<true>;

		Iterator begin() { return ++Iterator(this, -1); }

		Iterator end() { return Iterator(this, capacity()); }

		ConstIterator begin() const { return ++ConstIterator(this, -1); }

		ConstIterator end() const { return ConstIterator(this, capacity()); }
	};
}
#endif // AYR_BASE_TABLE_HPP