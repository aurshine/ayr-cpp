#ifndef AYR_DETAIL_HASH_BUCKET_HPP
#define AYR_DETAIL_HASH_BUCKET_HPP

#include <algorithm>
#include <functional>
#include <chrono>
#include <thread>

#include <ayr/printer.hpp>
#include <ayr/detail/Array.hpp>
#include <ayr/detail/hash.hpp>
#include <ayr/detail/RelationIterator.hpp>

namespace ayr
{
	template<typename T>
	class HashBucketImpl : public Object<HashBucketImpl<T>>
	{
		using self = HashBucketImpl<T>;
	public:
		using Value_t = T;

		HashBucketImpl() = default;

		virtual ~HashBucketImpl() = default;

		// 容量
		virtual c_size capacity() const = 0;

		// 克隆
		virtual self* clone() const = 0;

		// 尝试根据hash值获取元素，返回指针，如果没有则返回nullptr
		virtual Value_t* try_get(hash_t hashv) = 0;

		virtual const Value_t* try_get(hash_t hashv) const = 0;

		// 放置元素，不检查元素是否存在，不考虑是否超过容量, 返回指针
		virtual Value_t* set_store(const Value_t& store, hash_t hashv) = 0;

		virtual void expand(c_size new_capacity) noexcept = 0;

		virtual void clear() = 0;

		// 将hash值转换为索引
		virtual c_size hash2index(hash_t hashv) const { return hashv % capacity(); }

		// 是否包含元素
		virtual bool contains(hash_t hashv) const { return try_get(hashv) != nullptr; }

		// 放入元素，返回被放入的元素指针
		virtual Value_t* try_set(const Value_t& store, hash_t hashv)
		{
			Value_t* ret = try_get(hashv);
			if (ret == nullptr)
				ret = set_store(store, hashv);

			return ret;
		}
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
			other._move_dist = MOVED_NOTHING;
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
			other._move_dist = MOVED_NOTHING;
			return *this;
		}

		Value_t& value() { return _value; }

		const Value_t& value() const { return _value; }

		hash_t hashv() const { return _hashv; }

		bool is_managed() const { return _move_dist != MOVED_NOTHING; }

		bool hash_equal(hash_t hashv) const { return _hashv == hashv; }

		bool move_more_than(const self& other) const { return _move_dist > other._move_dist; }

		void add_move_dist() { ++_move_dist; }

		void swap(self& other)
		{
			std::swap(_move_dist, other._move_dist);
			std::swap(_hashv, other._hashv);
			std::swap(_value, other._value);
		}

	private:
		uint32_t _move_dist;

		hash_t _hashv;

		Value_t _value;

		constexpr static uint32_t MOVED_NOTHING = 0xFFFFFFFF;
	};


	template<typename T>
	class RobinHashBucket : public HashBucketImpl<T>
	{
		using self = RobinHashBucket<T>;

		using super = HashBucketImpl<T>;

	public:
		using Value_t = super::Value_t;

		using Manager_t = RobinManager<Value_t>;

		RobinHashBucket() : RobinHashBucket(0) {}

		RobinHashBucket(c_size size) : robin_managers_(size, Manager_t{}) { }

		RobinHashBucket(const RobinHashBucket& other) : robin_managers_(other.robin_managers_) {}

		RobinHashBucket(RobinHashBucket&& other) noexcept : robin_managers_(std::move(other.robin_managers_)) {}

		~RobinHashBucket() {}

		c_size capacity() const override { return robin_managers_.size(); }

		self* clone() const override { return new self(*this); }

		Value_t* try_get(hash_t hashv) override
		{
			for (c_size index = super::hash2index(hashv), i = 0; i < capacity(); index = (index + 1) % capacity(), ++i)
			{
				Manager_t& manager = robin_managers_.at(index);
				if (!manager.is_managed())
					break;
				if (manager.hash_equal(hashv))
					return &manager.value();
			}

			return nullptr;
		}

		const Value_t* try_get(hash_t hashv) const override
		{
			for (c_size index = super::hash2index(hashv), i = 0; i < capacity(); index = (index + 1) % capacity(), ++i)
			{
				const Manager_t& manager = robin_managers_.at(index);
				if (!manager.is_managed())
					break;
				if (manager.hash_equal(hashv))
					return &manager.value();
			}

			return nullptr;
		}

		Value_t* set_store(const Value_t& store, hash_t hashv)
		{
			Manager_t store_manager{ store, hashv, 0 };
			Value_t* ret = nullptr;
			for (c_size index = super::hash2index(hashv), i = 0; i < capacity(); index = (index + 1) % capacity(), ++i)
			{
				Manager_t& manager = robin_managers_.at(index);
				if (!manager.is_managed())
				{
					manager = std::move(store_manager);
					ret = &manager.value();
					break;
				}
				else if (store_manager.move_more_than(manager))
				{
					store_manager.swap(manager);
				}
				store_manager.add_move_dist();
			}

			error_assert(ret != nullptr, "RobinHashBucket: set_store failed, no empty slot found.");
			return ret;
		}

		void expand(c_size new_capacity) noexcept override
		{
			Array<Manager_t> temp_managers(std::move(robin_managers_));
			robin_managers_.resize(new_capacity);

			for (auto&& manager : temp_managers)
				set_store(manager.value(), manager.hashv());
		}

		void clear() override { robin_managers_.resize(0); }


		template<bool IsConst>
		class RobinHashBucketIterator : public Object<RobinHashBucketIterator<IsConst>>
		{
			using self = RobinHashBucketIterator;
		public:
			using Container_t = std::conditional_t<IsConst, const RobinHashBucket, RobinHashBucket>;

			using iterator_category = std::forward_iterator_tag;

			using value_type = std::conditional_t<IsConst, const Value_t, Value_t>;

			using difference_type = std::ptrdiff_t;

			using pointer = value_type*;

			using const_pointer = const value_type*;

			using reference = value_type&;

			using const_reference = const value_type&;

			RobinHashBucketIterator() : bucket_ptr_(nullptr), index_(0) {}

			RobinHashBucketIterator(Container_t* bucket_ptr, c_size index) : bucket_ptr_(bucket_ptr), index_(index) {}

			RobinHashBucketIterator(const self& other) : bucket_ptr_(other.bucket_ptr_), index_(other.index_) {}

			self& operator=(const self& other)
			{
				if (this == &other) return *this;

				bucket_ptr_ = other.bucket_ptr_;
				index_ = other.index_;
				return *this;
			}

			reference operator*() { return bucket_ptr_->robin_managers_[index_].value(); }

			const_reference operator*() const { return bucket_ptr_->robin_managers_[index_].value(); }

			pointer operator->() { return &bucket_ptr_->robin_managers_[index_].value(); }

			const_pointer operator->() const { return &bucket_ptr_->robin_managers_[index_].value(); }

			self& operator++()
			{
				while (++index_ < bucket_ptr_->capacity() && !bucket_ptr_->robin_managers_[index_].is_managed());
				return *this;
			}

			self operator++ (int)
			{
				self temp = *this;
				++*this;
				return temp;
			}

			bool __equals__(const self& other) const override
			{
				return bucket_ptr_ == other.bucket_ptr_ && index_ == other.index_;
			}
		private:
			Container_t* bucket_ptr_;

			c_size index_;
		};

		using Iterator = RobinHashBucketIterator<false>;

		using ConstIterator = RobinHashBucketIterator<true>;

		Iterator begin()
		{
			for (c_size i = 0; i < capacity(); ++i)
				if (robin_managers_.at(i).is_managed())
					return Iterator(this, i);
			return end();
		}

		ConstIterator begin() const
		{
			for (c_size i = 0; i < capacity(); ++i)
				if (robin_managers_.at(i).is_managed())
					return ConstIterator(this, i);
			return end();
		}

		Iterator end() { return Iterator(this, capacity()); }

		ConstIterator end() const { return ConstIterator(this, capacity()); }

	private:
		Array<Manager_t> robin_managers_;
	};
}
#endif