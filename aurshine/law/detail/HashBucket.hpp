#ifndef AYR_LAW_DETAIL_HASH_BUCKET_HPP
#define AYR_LAW_DETAIL_HASH_BUCKET_HPP

#include <algorithm>
#include <functional>
#include <chrono>
#include <thread>

#include <law/printer.hpp>
#include <law/detail/Array.hpp>
#include <law/detail/hash.hpp>
#include <law/detail/RelationIterator.hpp>

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

		RobinManager() : _move_dist(MOVED_NOTHING), _hashv(0), _value(1111111) {}

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


	template<IteratorLike I>
	class RobinHashBucketIterator : public Object<RobinHashBucketIterator<I>>
	{
		using self = RobinHashBucketIterator<I>;
	public:
		using Iterator = I;

		using Value_t = std::remove_reference_t<decltype(*std::declval<Iterator>())>::Value_t;

		using Reference_t = Value_t&;

		RobinHashBucketIterator(Iterator iter, Iterator end) : iter_(iter), end_(end) {}

		RobinHashBucketIterator(const self& other) : iter_(other.iter_), end_(other.end_) {}

		self& operator=(const self& other)
		{
			if (this == &other) return *this;

			iter_ = other.iter_;
			end_ = other.end_;
			return *this;
		}

		Reference_t operator*() { return iter_->value(); }

		const Reference_t operator*() const { return iter_->value(); }

		Value_t* operator->() { return &iter_->value(); }

		const Value_t* operator->() const { return &iter_->value(); }

		self& operator++()
		{
			while (++iter_ != end_ && !iter_->is_managed());
			return *this;
		}

		self operator++ (int)
		{
			Iterator temp = iter_;
			++*this;
			return self(temp);
		}

		bool __equals__(const self& other) const override
		{
			return iter_ == other.iter_;
		}
	private:
		Iterator iter_, end_;
	};


	template<typename T>
	class RobinHashBucket : public HashBucketImpl<T>
	{
		using self = RobinHashBucket<T>;

		using super = HashBucketImpl<T>;

	public:
		using Value_t = super::Value_t;

		using Manager_t = RobinManager<Value_t>;

		using Iterator = RobinHashBucketIterator<typename Array<Manager_t>::Iterator>;

		using ConstIterator = RobinHashBucketIterator<typename Array<Manager_t>::ConstIterator>;

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

		Iterator begin()
		{
			for (auto it = robin_managers_.begin(); it != robin_managers_.end(); ++it)
				if (it->is_managed())
					return Iterator(it, robin_managers_.end());
			return end();
		}

		Iterator end() { return Iterator(robin_managers_.end(), robin_managers_.end()); }

		ConstIterator begin() const
		{
			for (auto it = robin_managers_.begin(); it != robin_managers_.end(); ++it)
				if (it->is_managed())
					return ConstIterator(it, robin_managers_.end());
			return end();
		}

		ConstIterator end() const { return ConstIterator(robin_managers_.end(), robin_managers_.end()); }
	private:
		Array<Manager_t> robin_managers_;
	};
}
#endif