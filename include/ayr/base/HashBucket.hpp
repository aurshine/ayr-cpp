﻿#ifndef AYR_BASE_HASH_BUCKET_HPP
#define AYR_BASE_HASH_BUCKET_HPP

#include "Array.hpp"
#include "hash.hpp"
#include "bunit.hpp"

namespace ayr
{
	// 计算最合适的bucket数量
	def adapt_bucket_size(c_size base_size, double max_load_factor)
	{
		return roundup2(base_size / max_load_factor);
	}

	template<typename Derived, typename T>
	class HashBucketImpl : public Object<Derived>
	{
		using self = HashBucketImpl<Derived, T>;

		using super = Object<Derived>;
	public:
		using Value_t = T;

		HashBucketImpl() = default;

		~HashBucketImpl() = default;

		// 容量
		c_size capacity() const { NotImplementedError(std::format("{} not implemented capacity()", dtype(Derived))); return None<int>; };

		// 尝试根据hash值获取元素，返回指针，如果没有则返回nullptr
		Value_t* try_get(hash_t hashv) { NotImplementedError(std::format("{} not implemented try_get(hash_t)", dtype(Derived))); return nullptr; };

		const Value_t* try_get(hash_t hashv) const { NotImplementedError(std::format("{} not implemented try_get(hash_t) const", dtype(Derived))); return nullptr; };

		// 根据hash值删除元素
		void pop(hash_t hashv) { NotImplementedError(std::format("{} not implemented pop(hash_t)", dtype(Derived))); };

		// 扩容
		void expand(c_size new_capacity) noexcept { NotImplementedError(std::format("{} not implemented expand(c_size)", dtype(Derived))); };

		// 清空
		void clear() { NotImplementedError(std::format("{} not implemented clear()", dtype(Derived))); };

		// 将hash值转换为索引
		c_size hash2index(hash_t hashv) const { return hashv % super::derived().capacity(); }

		// 是否包含元素
		bool contains(hash_t hashv) const { return super::derived().try_get(hashv) != nullptr; }
	};

	template<typename T>
	class RobinManager : public Object<RobinManager<T>>
	{
		using self = RobinManager<T>;
	public:
		using Value_t = T;

		RobinManager(const Value_t& value, hash_t hashv, uint32_t move_dist) :
			_move_dist(move_dist),
			_hashv(hashv),
			_value(value) {}

		RobinManager(const RobinManager& other) :
			_move_dist(other._move_dist),
			_hashv(other._hashv),
			_value(other._value) {}

		RobinManager(RobinManager&& other) noexcept :
			_move_dist(other._move_dist),
			_hashv(other._hashv),
			_value(std::move(other._value)) {}

		self& operator=(const self& other)
		{
			if (this == &other) return *this;

			_move_dist = other._move_dist;
			_hashv = other._hashv;
			_value = other._value;
			return *this;
		}

		self& operator=(self&& other) noexcept
		{
			if (this == &other) return *this;

			_move_dist = other._move_dist;
			_hashv = other._hashv;
			_value = std::move(other._value);
			return *this;
		}

		Value_t& value() { return _value; }

		const Value_t& value() const { return _value; }

		hash_t hashv() const { return _hashv; }

		bool hash_equal(hash_t hashv) const { return _hashv == hashv; }

		// 移动距离是否大于other
		bool move_more_than(const self& other) const { return _move_dist > other._move_dist; }

		bool move_more_than(int move_dist) const { return _move_dist > move_dist; }

		void add_move_dist(int d = 1) { _move_dist += d; }

		// 重置移动距离
		void reset_move_dist() { _move_dist = 0; }

		void __swap__(self& other)
		{
			swap(_move_dist, other._move_dist);
			swap(_hashv, other._hashv);
			swap(_value, other._value);
		}
	private:
		int _move_dist;

		hash_t _hashv;

		Value_t _value;
	};

	template<typename T>
	class RobinHashBucket : public HashBucketImpl<RobinHashBucket<T>, T>
	{
		using self = RobinHashBucket<T>;

		using super = HashBucketImpl<self, T>;
	public:
		using Value_t = super::Value_t;

		using Manager_t = RobinManager<Value_t>;

		RobinHashBucket() : RobinHashBucket(0) {}

		RobinHashBucket(c_size size) : robin_managers_(size, nullptr) {}

		RobinHashBucket(const self& other) : RobinHashBucket(other.robin_managers_.size())
		{
			for (int i = 0, n = capacity(); i < n; ++i)
				if (other.robin_managers_.at(i) != nullptr)
					robin_managers_.at(i) = ayr_make<Manager_t>(*other.robin_managers_.at(i));
		}

		RobinHashBucket(self&& other) noexcept : robin_managers_(std::move(other.robin_managers_)) {}

		~RobinHashBucket() { clear(); }

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

		c_size capacity() const { return robin_managers_.size(); }

		// 获取合适的管理器地址
		// 1. 找到相同的hash值, 返回该管理器地址
		// 2. 找到距离最近的空闲管理器, 返回该管理器地址
		// 3. 没有空闲且相同hash值的管理器，返回nullptr
		Manager_t** try_get_manager(hash_t hashv)
		{
			c_size capacity_ = capacity();
			for (c_size index = super::hash2index(hashv), i = 0; i < capacity_; ++i)
			{
				Manager_t*& manager = robin_managers_.at((index + i) % capacity_);
				if (manager == nullptr || manager->hash_equal(hashv))
					return &manager;
			}
			return nullptr;
		}

		Manager_t* const* try_get_manager(hash_t hashv) const
		{
			c_size capacity_ = capacity();
			for (c_size index = super::hash2index(hashv), i = 0; i < capacity_; ++i)
			{
				Manager_t* const& manager = robin_managers_.at((index + i) % capacity_);
				if (manager == nullptr || manager->hash_equal(hashv))
					return &manager;
			}
			return nullptr;
		}

		Value_t* try_get(hash_t hashv)
		{
			Manager_t** manager = try_get_manager(hashv);
			if (manager == nullptr || *manager == nullptr)
				return nullptr;
			return &((**manager).value());
		}

		const Value_t* try_get(hash_t hashv) const
		{
			Manager_t* const* manager = try_get_manager(hashv);
			if (manager == nullptr || *manager == nullptr)
				return nullptr;
			return &((**manager).value());
		}

		// 根据hashv添加值，返回值的指针
		// 默认认为hashv不存在
		// 若bucket已满，则抛出异常
		template<typename V>
		Value_t* set_value(V&& value, hash_t hashv)
		{
			Manager_t* new_manager = ayr_make<Manager_t>(std::forward<V>(value), hashv, 0);

			return set_value(new_manager);
		}

		Value_t* set_value(Manager_t* new_manager)
		{
			c_size capacity_ = capacity();
			for (c_size index = super::hash2index(new_manager->hashv()), i = 0; i < capacity_; ++i)
			{
				Manager_t*& manager = robin_managers_.at((index + i) % capacity_);
				if (manager == nullptr)
				{
					manager = new_manager;
					return &manager->value();
				}
				else if (new_manager->move_more_than(*manager))
				{
					ayr::swap(manager, new_manager);
				}
				new_manager->add_move_dist();
			}

			RuntimeError("RobinHashBucket set_value failed, bucekt is full");
			return nullptr;
		}

		void pop(hash_t hashv)
		{
			c_size capacity_ = capacity();
			Manager_t** manager = try_get_manager(hashv);
			if (manager == nullptr || *manager == nullptr)
				return;

			ayr_desloc(*manager, 1);
			*manager = nullptr;

			c_size cur_idx = std::distance(robin_managers_.data(), manager);

			while (true)
			{
				c_size next_idx = (cur_idx + 1) % capacity_;
				Manager_t*& cur_manager = robin_managers_.at(cur_idx);
				Manager_t*& next_manager = robin_managers_.at(next_idx);

				if (next_manager != nullptr && next_manager->move_more_than(0))
				{
					next_manager->add_move_dist(-1);
					cur_manager = next_manager;
					next_manager = nullptr;
				}
				else
					break;

				cur_idx = next_idx;
			}
		}

		void expand(c_size new_capacity) noexcept
		{
			Array<Manager_t*> temp_managers(std::move(robin_managers_));
			robin_managers_.resize(new_capacity);

			for (Manager_t* manager : temp_managers)
				if (manager != nullptr)
				{
					manager->reset_move_dist();
					set_value(manager);
				}

		}

		void clear()
		{
			for (Manager_t* manager : robin_managers_)
				if (manager != nullptr)
					ayr_desloc(manager, 1);
			robin_managers_.resize(0);
		}

		void __swap__(self& other)
		{
			swap(robin_managers_, other.robin_managers_);
		}

		template<bool IsConst>
		class RobinHashBucketIterator :
			public IteratorInfo<RobinHashBucketIterator<IsConst>, add_const_t<IsConst, Array<Manager_t*>>, std::forward_iterator_tag, add_const_t<IsConst, Value_t>>
		{
			using self = RobinHashBucketIterator;

			using ItInfo = IteratorInfo<self, add_const_t<IsConst, Array<Manager_t*>>, std::forward_iterator_tag, add_const_t<IsConst, Value_t>>;
		public:
			RobinHashBucketIterator() : bucket_ptr_(nullptr), index_(0) {}

			RobinHashBucketIterator(ItInfo::container_type* bucket_ptr, c_size index) : bucket_ptr_(bucket_ptr), index_(index) {}

			RobinHashBucketIterator(const self& other) : bucket_ptr_(other.bucket_ptr_), index_(other.index_) {}

			self& operator=(const self& other)
			{
				if (this == &other) return *this;

				bucket_ptr_ = other.bucket_ptr_;
				index_ = other.index_;
				return *this;
			}

			ItInfo::reference operator*() const { return bucket_ptr_->at(index_)->value(); }

			ItInfo::pointer operator->() const { return &bucket_ptr_->at(index_)->value(); }

			self& operator++()
			{
				++index_;
				while (index_ < bucket_ptr_->size() && bucket_ptr_->at(index_) == nullptr) ++index_;
				return *this;
			}

			self operator++ (int)
			{
				self temp = *this;
				++*this;
				return temp;
			}

			bool __equals__(const self& other) const
			{
				return bucket_ptr_ == other.bucket_ptr_ && index_ == other.index_;
			}
		private:
			ItInfo::container_type* bucket_ptr_;

			c_size index_;
		};

		using Iterator = RobinHashBucketIterator<false>;

		using ConstIterator = RobinHashBucketIterator<true>;

		Iterator begin()
		{
			for (c_size i = 0; i < capacity(); ++i)
				if (robin_managers_.at(i) != nullptr)
					return Iterator(&this->robin_managers_, i);
			return end();
		}

		ConstIterator begin() const
		{
			for (c_size i = 0; i < capacity(); ++i)
				if (robin_managers_.at(i) != nullptr)
					return ConstIterator(&this->robin_managers_, i);
			return end();
		}

		Iterator end() { return Iterator(&this->robin_managers_, capacity()); }

		ConstIterator end() const { return ConstIterator(&this->robin_managers_, capacity()); }
	private:
		Array<Manager_t*> robin_managers_;
	};
}
#endif