#pragma once
#include <law/Array.hpp>
#include <law/hash.hpp>


namespace ayr
{
	// 键值对
	template<Hashable K, typename V>
	struct KeyValue : public Object
	{
		KeyValue() : key(), value() {}

		KeyValue(const K& key, const V& value) : key(key), value(value) {}

		KeyValue(K&& key, V&& value) : key(std::move(key)), value(std::move(value)) {}

		KeyValue(const KeyValue& other) : key(other.key), value(other.value) {}

		KeyValue(KeyValue&& other) : key(std::move(other.key)), value(std::move(other.value)) {}

		size_t key_hash() const { return ayrhash(key); }

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


	// 哈希字典
	template<Hashable K, typename V, typename C = Creator<KeyValue<K, V>>>
	class Dict : public Object
	{
		using KV_t = KeyValue<K, V>;

		using Bucket_t = Array<KV_t*>;

		using Skip_t = Array<uint32_t>;
	public:
		Dict() : Dict(31) {}

		Dict(c_size bucket_size) : bucket_(bucket_size, nullptr), skip_list_(bucket_size, 0), size_(0) {}

		Dict(std::initializer_list<KV_t>&& kv_list) : bucket_(kv_list.size() / 0.7)
		{
			for (auto&& kv : kv_list)
				setkv2bucket(bucket_, skip_list_, creator_(std::move(kv)));
		}

		// 重载[]运算符, 若key不存在, 则创建并返回一个默认值
		V& operator[](const K& key)
		{
			if (!contains(key))
				setkv2bucket(bucket_, skip_list_, creator_(key, V()));

			return get(key);
		}

		// 重载[]运算符, key 必须存在, 否则KeyError
		const V& operator[](const K& key) const { return get(key); }


		// 获得key对应的value, 若key不存在, 则抛出KeyError
		V& get(const K& key)
		{
			c_size start_index = get_hash_index(bucket_, key);
			while (bucket_[start_index] != nullptr)
			{
				if (bucket_[start_index]->key_equals(key))
					return bucket_[start_index]->value;

				start_index = (start_index + 1) % bucket_.size();
			}

			KeyError("Key not found");
			return None<V>;
		}

		// 获得key对应的value, 若key不存在, 则抛出KeyError
		const V& get(const K& key) const
		{
			c_size start_index = get_hash_index(bucket_, key);
			while (bucket_[start_index] != nullptr)
			{
				if (bucket_[start_index]->key_equals(key))
					return bucket_[start_index]->value;

				start_index = (start_index + 1) % bucket_.size();
			}

			KeyError("Key not found in dict");
			return None<V>;
		}

		// 获得key对应的value, 若key不存在, 则返回default_value
		V& get(const K& key, V& default_value)
		{
			c_size start_index = get_hash_index(bucket_, key);
			while (bucket_[start_index] != nullptr)
			{
				if (bucket_[start_index]->key_equals(key))
					return bucket_[start_index]->value;

				start_index = (start_index + 1) % bucket_.size();
			}

			return default_value;
		}

		const V& get(const K& key, const V& default_value) const
		{
			c_size start_index = get_hash_index(bucket_, key);
			while (bucket_[start_index] != nullptr)
			{
				if (bucket_[start_index]->key_equals(key))
					return bucket_[start_index]->value;

				start_index = (start_index + 1) % bucket_.size();
			}

			return default_value;
		}

		bool contains(const K& key) const
		{
			c_size start_index = get_hash_index(bucket_, key);
			while (bucket_[start_index] != nullptr)
			{
				if (bucket_[start_index]->key_equals(key))
					return true;

				start_index = (start_index + 1) % bucket_.size();
			}

			return false;
		}


		Dict& setdefault(const K& key, const V& default_value)
		{
			if (!contains(key))
				this->operator[](key) = default_value;

			return *this;
		}

		double load_factor() const { return size_ / bucket_.size(); }

		void expand(size_t expand_size)
		{
			Array<KV_t*> new_bucket(expand_size, nullptr);
			skip_list_.relloc(expand_size);
			skip_list_.fill(0);

			for (KV_t* kv : bucket_)
				if (kv != nullptr)
					setkv2bucket(new_bucket, skip_list_, kv);

			bucket_ = std::move(new_bucket);
			new_bucket.fill(nullptr);
		}
	private:
		// 得到key的hash值在bucket中的索引
		c_size get_hash_index(const Bucket_t& bucket, const K& key) const { return ayrhash(key) % bucket.size(); }

		// 向bucket中添加元素, 并对照更新skip_list
		void setkv2bucket(Bucket_t& bucket, Skip_t& skip_list, KV_t* kv)
		{
			size_t start_index = get_hash_index(bucket, kv->key);

			uint32_t skipcnt = 0;
			while (bucket[start_index] != nullptr)
			{
				if (skipcnt > skip_list[start_index])
				{
					std::swap(kv, bucket[start_index]);
					std::swap(skipcnt, skip_list[start_index]);
				}

				start_index = (start_index + 1) % bucket.size();
				skipcnt++;
			}

			bucket[start_index] = kv;
			skip_list[start_index] = skipcnt;
		}

		// 添加元素到bucket中, 并更新size_
		void addkv_at(KV_t* kv, c_size index)
		{
			bucket_[index] = kv;
			size_++;
		}
	private:
		Bucket_t bucket_;

		Skip_t skip_list_; // 记录每个key离自己原本的位置的距离

		size_t size_;

		C creator_;
	};
}