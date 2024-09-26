#ifndef AYR_LAW_DICT_HPP
#define AYR_LAW_DICT_HPP

#include <law/detail/hash.hpp>
#include <law/Array.hpp>
#include <law/ayr_memory.hpp>


namespace ayr
{
	// 键值对
	template<Hashable K, typename V>
	struct KeyValue : public Object<KeyValue<K, V>>
	{
		using self = KeyValue<K, V>;

		KeyValue() : key(), value() {}

		KeyValue(const K& key, const V& value) : key(key), value(value) {}

		KeyValue(K&& key, V&& value) : key(std::move(key)), value(std::move(value)) {}

		KeyValue(const self& other) : key(other.key), value(other.value) {}

		KeyValue(self&& other) : key(std::move(other.key)), value(std::move(other.value)) {}

		size_t key_hash() const { return ayrhash(key); }

		bool key_equals(const K& other) const { return key == other; }


		self& operator=(const self& other)
		{
			key = other.key;
			value = other.value;
			return *this;
		}


		self& operator=(self&& other) noexcept
		{
			key = std::move(other.key);
			value = std::move(other.value);
			return *this;
		}

		K key;

		V value;
	};


	// 哈希字典
	template<Hashable K, typename V, typename C = Creator<std::pair<hash_t, V>>>
	class Dict : public Object<Dict<K, V, C>>
	{
		using self = Dict<K, V, C>;

		using super = Object;
	public:
		using Key_t = K;

		using Value_t = V;

		using KV_t = KeyValue<Key_t, Value_t>;

		using BucketValue_t = std::pair<hash_t, Value_t>;

		using Bucket_t = Array<BucketValue_t*>;

		using Dist_t = uint32_t;

		using SkipList_t = Array<Dist_t>;

		constexpr static c_size DEF_BUCKET_SIZE = 31;
	public:
		Dict() : Dict(DEF_BUCKET_SIZE) {}

		Dict(c_size bucket_size) : bucket_(std::max(bucket_size, DEF_BUCKET_SIZE), nullptr), skip_list_(std::max(bucket_size, DEF_BUCKET_SIZE), 0) {}

		Dict(std::initializer_list<KV_t>&& kv_list) : Dict(kv_list.size() / 0.7)
		{
			for (auto&& kv : kv_list)
				setkv2bucket(std::move(kv.key), std::move(kv.value));
		}

		Dict(const Dict& other) : Dict(other.size() / 0.7)
		{
			for (auto&& kv : other)
				setkv2bucket(kv.key, kv.value);
		}

		Dict(Dict&& other)
		{
			this->bucket_ = std::move(other.bucket_);
			this->skip_list_ = std::move(other.skip_list_);
			this->keys_ = std::move(other.keys_);
			this->creator_ = std::move(other.creator_);
		}

		Dict& operator=(const Dict& other)
		{
			if (this == &other) return *this;
			for (auto&& kv : other)
				setkv2bucket(kv.key, kv.value);
		}


		Dict& operator=(Dict&& other) noexcept
		{
			if (this == &other) return *this;

			this->bucket_ = std::move(other.bucket_);
			this->skip_list_ = std::move(other.skip_list_);
			this->keys_ = std::move(other.keys_);
			this->creator_ = std::move(other.creator_);
			return *this;
		}

		// key-value对的数量
		c_size size() const { return keys_.size(); }

		// key是否存在于字典中
		bool contains(const K& key) const { return get_kv(key) != nullptr; }

		// 得到字典中所有的key
		Array<K> keys() const { return keys_.to_array(); }

		// 内存由该Dict内部管理, 创建一个KV_t对象，返回其指针
		// KV_t* mk_kv(const K& key, const V& value) { return creator_(key, value); }

		// 重载[]运算符, key 必须存在, 否则KeyError
		const V& operator[](const K& key) const { return get(key); }

		// 重载[]运算符, 若key不存在, 则创建并返回一个默认值
		V& operator[](const K& key)
		{
			if (!contains(key))
				setkv2bucket(key, V());
			return get(key);
		}

		// 获得key对应的value, 若key不存在, 则抛出KeyError
		V& get(const K& key)
		{
			BucketValue_t* kv = get_kv(key);
			if (kv != nullptr) return kv->second;

			KeyError("Key not found in dict");
			return None<V>;
		}

		// 获得key对应的value, 若key不存在, 则抛出KeyError
		const V& get(const K& key) const
		{
			BucketValue_t* kv = get_kv(key);
			if (kv != nullptr) return kv->second;

			KeyError("Key not found in dict");
			return None<V>;
		}

		// 获得key对应的value, 若key不存在, 则返回default_value
		V& get(const K& key, V& default_value)
		{
			BucketValue_t* kv = get_kv(key);
			if (kv != nullptr) return kv->second;

			return default_value;
		}

		// 获得key对应的value, 若key不存在, 则返回default_value
		const V& get(const K& key, const V& default_value) const
		{
			BucketValue_t* kv = get_kv(key);
			if (kv != nullptr) return kv->second;

			return default_value;
		}

		// 若key不存在, 则添加一个默认值
		Dict& setdefault(const K& key, const V& default_value)
		{
			if (!contains(key)) setkv2bucket(key, default_value);

			return *this;
		}

		// 根据传入的字典更新字典
		Dict& update(const Dict& other)
		{
			if (size() + other.size() > bucket_.size() * 0.7)
				expand((size() + other.size()) * 1.5);

			for (auto&& kv : other)
				if (!contains(kv.key))
					setkv2bucket(kv.key, kv.value);
				else
					get(kv.key) = kv.value;

			return *this;
		}

		CString __str__() const override
		{
			std::stringstream stream;
			stream << "{";
			for (c_size i = 0; i < keys_.size(); ++i)
			{
				if (i != 0) stream << ", ";
				stream << keys_[i] << ":" << get(keys_[i]);
			}

			stream << "}";

			return CString(stream.str());
		}
	private:
		// 得到key的hash值在bucket中的索引
		c_size get_hash_index(const Bucket_t& bucket, const K& key) const { return ayrhash(key) % bucket.size(); }

		c_size get_hashv_index(const Bucket_t& bucket, hash_t hashv) const { return hashv % bucket.size(); }

		// 根据key获得存在dict里的KeyValue对象的指针, key不存在返回nullptr
		BucketValue_t* get_kv(const K& key) const
		{
			hash_t hashv = ayrhash(key);
			c_size start_index = get_hashv_index(bucket_, hashv);

			while (bucket_[start_index] != nullptr)
			{
				if (bucket_[start_index]->first == hashv)
					return bucket_[start_index];

				start_index = (start_index + 1) % bucket_.size();
			}

			return nullptr;
		}

		// 向bucket中添加元素, 并对照更新skip_list
		void setkv2bucket_impl(Bucket_t& bucket, SkipList_t& skip_list, BucketValue_t* bv)
		{
			size_t start_index = get_hashv_index(bucket, bv->first);

			Dist_t skipcnt = 0;
			while (bucket[start_index] != nullptr)
			{
				if (skipcnt > skip_list[start_index])
				{
					std::swap(bv, bucket[start_index]);
					std::swap(skipcnt, skip_list[start_index]);
				}

				start_index = (start_index + 1) % bucket.size();
				++skipcnt;
			}

			bucket[start_index] = bv;
			skip_list[start_index] = skipcnt;
		}

		// 向字典中添加元素，不会检查key是否已经存在
		void setkv2bucket(const Key_t& k, const Value_t& v)
		{
			BucketValue_t* bv = creator_(std::make_pair(ayrhash(k), v));
			if (1.0 * size() / bucket_.size() > 0.7) expand(bucket_.size() * 2);

			setkv2bucket_impl(bucket_, skip_list_, bv);

			keys_.append(k);
		}

		void setkv2bucket(Key_t&& k, Value_t&& v)
		{
			BucketValue_t* bv = creator_(std::make_pair(ayrhash(std::move(k)), std::move(v)));
			if (1.0 * size() / bucket_.size() > 0.7) expand(bucket_.size() * 2);

			setkv2bucket_impl(bucket_, skip_list_, bv);

			keys_.append(k);
		}


		void expand(size_t expand_size)
		{
			Bucket_t new_bucket(expand_size, nullptr);
			SkipList_t new_skip_list(expand_size, 0);

			for (auto&& bv : bucket_)
				if (bv != nullptr)
					setkv2bucket_impl(new_bucket, new_skip_list, bv);

			bucket_ = std::move(new_bucket);
			skip_list_ = std::move(new_skip_list);
		}
	private:
		Bucket_t bucket_;

		SkipList_t skip_list_; // 记录每个key离自己原本的位置的距离

		DynArray<K> keys_; // 用于迭代

		C creator_;
	};
}
#endif