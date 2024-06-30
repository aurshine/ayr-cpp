#pragma once
#include <law/object.hpp>
#include <law/ayr_concepts.hpp>
#include <law/Array.hpp>


namespace ayr
{
	// 用于取模的质数常数
	constexpr size_t HASH_PRIME_1 = 1331, HASH_PRIME_2 = 10007;


	template<ayr::DerivedAyr T>
	struct std::hash<T>
	{
		size_t operator()(const T& one) const { return one.__hash__(); }
	};


	// 键值对
	template<Hashable K, typename V>
	struct KeyValue : public Object
	{
		KeyValue(const K& key, const V& value) : key(key), value(value) {}

		KeyValue(K&& key, V&& value) : key(std::move(key)), value(std::move(value)) {}

		KeyValue(const KeyValue& other) : key(other.key), value(other.value) {}

		KeyValue(KeyValue&& other) : key(std::move(other.key)), value(std::move(other.value)) {}

		size_t key_hash() const { return std::hash<K>()(key); }

		bool key_equals(const K& other) const { return key == other; }


		KeyValue& operator=(const KeyValue& other)
		{
			key = other.key;
			value = other.value;
			return *this;
		}


		KeyValue& operator=(KeyValue&& other)
		{
			key = std::move(other.key);
			value = std::move(other.value);
			return *this;
		}


		K key;

		V value;
	};

	template<Hashable K, typename V>
	KeyValue<K, V> mk_kv(const K& key, const V& value) { return KeyValue(key, value); }


	template<typename V>
	struct ValueInplace : public Object
	{
		ValueInplace(V* v_ptr) : value_ptr(v_ptr) {}

		void check_nullptr() const
		{
			if (value_ptr == nullptr)
				error_assert(false, "ValueError: value is not exist");
		}

		V* operator-> ()
		{
			check_nullptr();
			return value_ptr;
		}

		const V* operator-> () const
		{
			check_nullptr();
			return value_ptr;
		}


		V& operator* ()
		{
			check_nullptr();
			return *value_ptr;
		}


		const V& operator* () const
		{
			check_nullptr();
			return *value_ptr;
		}


		operator V() const
		{
			check_nullptr();
			return *value_ptr;
		}

		ValueInplace& operator=(const V& other)
		{
			check_nullptr();
			*value_ptr = other;
			return *this;
		}

		V* value_ptr;
	};


	// 哈希字典
	template<Hashable K, typename V>
	class Dict : public Object
	{
		using Bucket_t = Array<KeyValue<K, V>*>;

		using KV_t = KeyValue<K, V>;

	public:
		Dict(c_size bucket_size) : bucket_(bucket_size) {}

		Dict() : Dict(31) {}

		Dict(std::initializer_list<KV_t>&& kv_list)
			: bucket_(kv_list.size() * 3 / 2)
		{
			for (auto&& kv : kv_list)
			{
				KeyValue<K, V>* kv_ptr = new KeyValue<K, V>(std::move(kv));

			}
		}

		~Dict() { release(); }


		const V& operator[](const K& key) const
		{
			size_t hash_v = std::hash<K>()(key);
			size_t index1 = hash_v % HASH_PRIME_1, idnex2 = hash_v % HASH_PRIME_2;
			for (size_t i = 0; i < bucket_.size(); ++i)
			{
				c_size index = (index1 + i * idnex2) % bucket_.size();
				if (bucket_[index] == nullptr)
					break;

				if (bucket_[index]->key_equals(key))
					return bucket_[index]->value;
			}

			error_assert(false, std::format("KeyError: key {} not found in dict"), key);
			return None<V>;
		}


		V& operator[](const K& key)
		{
			size_t hash_v = std::hash<K>()(key);
			size_t index1 = hash_v % HASH_PRIME_1, idnex2 = hash_v % HASH_PRIME_2;
			for (size_t i = 0; i < bucket_.size(); ++i)
			{
				c_size index = (index1 + i * idnex2) % bucket_.size();
				if (bucket_[index] == nullptr)
					break;

				if (bucket_[index]->key_equals(key))
					return bucket_[index]->value;
			}

			error_assert(false, std::format("KeyError: key {} not found in dict"), key);
			return None<V>;
		}


	protected:
		// 计算hash值对应的桶下标，如果所有对应下标被占用，返回-1
		c_size calc_empty_index_or_failure(size_t hash_v, const Bucket_t& bucket) const
		{
			size_t index1 = hash_v % HASH_PRIME_1, idnex2 = hash_v % HASH_PRIME_2;
			for (size_t i = 0; i < bucket.size(); ++i)
			{
				c_size index = (index1 + i * idnex2) % bucket.size();
				if (bucket[index] == nullptr)
					return index;
			}

			return -1;
		}


		// 移动元素到新的桶，如果失败，返回false
		bool move_bucket(const Bucket_t& old_bucket, Bucket_t& new_bucket)
		{
			for (auto&& kv_ptr : old_bucket)
			{
				if (kv_ptr == nullptr)
					continue;

				c_size index = calc_empty_index_or_failure(kv_ptr->key_hash(), new_bucket);

				if (index == -1)
					return false;

				new_bucket[index] = kv_ptr;
			}

			return true;
		}


		// 桶扩容
		void expend_bucket(c_size new_size)
		{
			Bucket_t new_bucket = Bucket_t(new_size, nullptr);

			// 没找到空闲位置，递归扩容
			if (!move_bucket(bucket_, new_bucket))
			{
				new_bucket.release();
				expend_bucket(new_size + std::min<c_size>(131, new_size));
			}
		}


		c_size calc_empty_index(size_t hash_v) const
		{
			while (true)
			{
				c_size index = calc_empty_index_or_failure(hash_v, bucket_);
				if (index == -1)
					expend_bucket(bucket_.size() * 2);
				else
					return index;
			}
		}


		// 释放内存
		void release()
		{
			for (auto&& kv_ptr : bucket_)
				if (kv_ptr != nullptr)
					delete kv_ptr;
		}

		Bucket_t bucket_;

		c_size size_ = 0;
	};
}