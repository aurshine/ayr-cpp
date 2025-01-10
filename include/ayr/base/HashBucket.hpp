#ifndef AYR_BASE_HASH_BUCKET_HPP
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

		RobinManager() : _move_dist(MOVED_NOTHING), _hashv(0), _value() {}

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
			_value(std::move(other._value))
		{
			other.unmanage();
		}

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
			other.unmanage();
			return *this;
		}

		Value_t& value() { return _value; }

		const Value_t& value() const { return _value; }

		hash_t hashv() const { return _hashv; }

		// 是否被管理
		bool is_managed() const { return _move_dist != MOVED_NOTHING; }

		// 取消管理
		void unmanage() { _move_dist = MOVED_NOTHING; }

		bool hash_equal(hash_t hashv) const { return _hashv == hashv; }

		// 移动距离是否大于other
		bool move_more_than(const self& other) const { return _move_dist > other._move_dist; }

		bool move_more_than(int move_dist) const { return _move_dist > move_dist; }

		void add_move_dist(int d = 1) { _move_dist += d; }

		void swap(self& other)
		{
			std::swap(_move_dist, other._move_dist);
			std::swap(_hashv, other._hashv);
			std::swap(_value, other._value);
		}
	private:
		int _move_dist;

		hash_t _hashv;

		Value_t _value;

		constexpr static int MOVED_NOTHING = INT_MAX;
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

		RobinHashBucket(c_size size) : robin_managers_(size) {}

		RobinHashBucket(const RobinHashBucket& other) : robin_managers_(other.robin_managers_) {}

		RobinHashBucket(RobinHashBucket&& other) noexcept : robin_managers_(std::move(other.robin_managers_)) {}

		self& operator=(const self& other)
		{
			if (this == &other) return *this;

			robin_managers_ = other.robin_managers_;
			return *this;
		}

		self& operator=(self&& other) noexcept
		{
			if (this == &other) return *this;

			robin_managers_ = std::move(other.robin_managers_);
			return *this;
		}

		c_size capacity() const { return robin_managers_.size(); }

		// 获取合适的管理器地址
		// 1. 找到相同的hash值, 返回该管理器地址
		// 2. 找到距离最近的空闲管理器, 返回该管理器地址
		// 3. 没有空闲且相同hash值的管理器，返回nullptr
		Manager_t* try_get_manager(hash_t hashv)
		{
			c_size capacity_ = capacity();
			for (c_size index = super::hash2index(hashv), i = 0; i < capacity_; ++i)
			{
				Manager_t& manager = robin_managers_.at((index + i) % capacity_);
				if (!manager.is_managed() || manager.hash_equal(hashv))
					return &manager;
			}
			return nullptr;
		}

		const Manager_t* try_get_manager(hash_t hashv) const
		{
			c_size capacity_ = capacity();
			for (c_size index = super::hash2index(hashv), i = 0; i < capacity_; ++i)
			{
				const Manager_t& manager = robin_managers_.at((index + i) % capacity_);
				if (!manager.is_managed() || manager.hash_equal(hashv))
					return &manager;
			}
			return nullptr;
		}

		Value_t* try_get(hash_t hashv)
		{
			Manager_t* manager = try_get_manager(hashv);
			if (manager == nullptr || !manager->is_managed())
				return nullptr;
			return &manager->value();
		}

		const Value_t* try_get(hash_t hashv) const
		{
			const Manager_t* manager = try_get_manager(hashv);
			if (manager == nullptr || !manager->is_managed())
				return nullptr;
			return &manager->value();
		}

		// 根据hashv添加值，返回值的指针
		// 默认认为hashv不存在
		// 若bucket已满，则抛出异常
		template<typename V>
		Value_t* set_value(V&& value, hash_t hashv)
		{
			Manager_t value_manager{ std::forward<V>(value), hashv, 0 };
			Value_t* ret = nullptr;
			c_size capacity_ = capacity();
			for (c_size index = super::hash2index(hashv), i = 0; i < capacity_; ++i)
			{
				Manager_t& manager = robin_managers_.at((index + i) % capacity_);
				if (!manager.is_managed())
				{
					manager = std::move(value_manager);
					ret = &manager.value();
					break;
				}
				else if (value_manager.move_more_than(manager))
				{
					value_manager.swap(manager);
				}
				value_manager.add_move_dist();
			}

			if (ret == nullptr)
				RuntimeError("RobinHashBucket set_value failed, bucekt is full");

			return ret;
		}

		void pop(hash_t hashv)
		{
			c_size capacity_ = capacity();
			Manager_t* manager = try_get_manager(hashv);
			if (manager == nullptr || !manager->is_managed())
				return;

			c_size cur_idx = std::distance(robin_managers_.data(), manager);
			while (true)
			{
				c_size next_idx = (cur_idx + 1) % capacity_;
				Manager_t& cur_manager = robin_managers_.at(cur_idx);
				Manager_t& next_manager = robin_managers_.at(next_idx);
				if (next_manager.is_managed() && next_manager.move_more_than(0))
				{
					next_manager.add_move_dist(-1);
					cur_manager = std::move(next_manager);
				}
				else
				{
					cur_manager.unmanage();
					break;
				}
				cur_idx = next_idx;
			}
		}

		void expand(c_size new_capacity) noexcept
		{
			Array<Manager_t> temp_managers(std::move(robin_managers_));
			robin_managers_.resize(new_capacity);
			for (Manager_t& manager : temp_managers)
				if (manager.is_managed())
					set_value(manager.value(), manager.hashv());
		}

		void clear() { robin_managers_.resize(0); }

		template<bool IsConst>
		class RobinHashBucketIterator :
			public IteratorInfo<RobinHashBucketIterator<IsConst>, add_const_t<IsConst, Array<Manager_t>>, std::forward_iterator_tag, add_const_t<IsConst, Value_t>>
		{
			using self = RobinHashBucketIterator;

			using ItInfo = IteratorInfo<self, add_const_t<IsConst, Array<Manager_t>>, std::forward_iterator_tag, add_const_t<IsConst, Value_t>>;
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

			ItInfo::reference operator*() const { return bucket_ptr_->at(index_).value(); }

			ItInfo::pointer operator->() const { return &bucket_ptr_->at(index_).value(); }

			self& operator++()
			{
				++index_;
				while (index_ < bucket_ptr_->size() && !bucket_ptr_->at(index_).is_managed()) ++index_;
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
				if (robin_managers_.at(i).is_managed())
					return Iterator(&this->robin_managers_, i);
			return end();
		}

		ConstIterator begin() const
		{
			for (c_size i = 0; i < capacity(); ++i)
				if (robin_managers_.at(i).is_managed())
					return ConstIterator(&this->robin_managers_, i);
			return end();
		}

		Iterator end() { return Iterator(&this->robin_managers_, capacity()); }

		ConstIterator end() const { return ConstIterator(&this->robin_managers_, capacity()); }
	private:
		Array<Manager_t> robin_managers_;
	};
}
#endif