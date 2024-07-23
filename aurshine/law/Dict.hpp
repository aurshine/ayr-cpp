#pragma once
#include <law/Array.hpp>
#include <law/detail/hash.hpp>
#include <law/ayr_memory.hpp>


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


		KeyValue& operator=(KeyValue&& other) noexcept
		{
			key = std::move(other.key);
			value = std::move(other.value);
			return *this;
		}

		K key;

		V value;
	};


	template<Hashable K, typename V, typename Creator>
	class DictGetItem;


	// 哈希字典
	template<Hashable K, typename V, typename C = Creator<KeyValue<K, V>>>
	class Dict : public IndexContainer<Dict<K, V, C>, KeyValue<K, V>, DictGetItem<K, V, C>>
	{
		using self = Dict<K, V, C>;

		using super = IndexContainer<self, KeyValue<K, V>>;
	public:
		using KV_t = KeyValue<K, V>;

		using Bucket_t = Array<KV_t*>;

		using Dist_t = uint32_t;

		using SkipList_t = Array<Dist_t>;

		constexpr static c_size DEF_BUCKET_SIZE = 31;

		friend class DictGetItem<K, V, C>;
	public:
		Dict() : Dict(DEF_BUCKET_SIZE) {}

		Dict(c_size bucket_size) : bucket_(std::max(bucket_size, DEF_BUCKET_SIZE), nullptr), skip_list_(std::max(bucket_size, DEF_BUCKET_SIZE), 0) {}

		Dict(std::initializer_list<KV_t>&& kv_list) : Dict(kv_list.size() / 0.7)
		{
			for (auto&& kv : kv_list)
				setkv2bucket(creator_(std::move(kv)));
		}

		Dict(const Dict& other) : Dict(other.size() / 0.7)
		{
			for (auto&& kv : other)
				setkv2bucket(creator_(kv));
		}

		Dict(Dict&& other) : Dict(other.size() / 0.7)
		{
			for (auto&& kv : other)
				setkv2bucket(creator_(std::move(kv)));
		}


		Dict& operator=(const Dict& other)
		{
			if (this == &other) return *this;
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
		KV_t* mk_kv(const K& key, const V& value) { return creator_(key, value); }

		// 重载[]运算符, key 必须存在, 否则KeyError
		const V& operator[](const K& key) const { return get(key); }

		// 重载[]运算符, 若key不存在, 则创建并返回一个默认值
		V& operator[](const K& key)
		{
			if (!contains(key)) setkv2bucket(creator_(key, V()));

			return get(key);
		}

		// 获得key对应的value, 若key不存在, 则抛出KeyError
		V& get(const K& key)
		{
			KV_t* kv = get_kv(key);
			if (kv != nullptr) return kv->value;

			KeyError("Key not found in dict");
			return None<V>;
		}

		// 获得key对应的value, 若key不存在, 则抛出KeyError
		const V& get(const K& key) const
		{
			KV_t* kv = get_kv(key);
			if (kv != nullptr) return kv->value;

			KeyError("Key not found in dict");
			return None<V>;
		}

		// 获得key对应的value, 若key不存在, 则返回default_value
		V& get(const K& key, V& default_value)
		{
			KV_t* kv = get_kv(key);
			if (kv != nullptr) return kv->value;

			return default_value;
		}

		// 获得key对应的value, 若key不存在, 则返回default_value
		const V& get(const K& key, const V& default_value) const
		{
			KV_t* kv = get_kv(key);
			if (kv != nullptr) return kv->value;

			return default_value;
		}

		// 若key不存在, 则添加一个默认值
		Dict& setdefault(const K& key, const V& default_value)
		{
			if (!contains(key)) setkv2bucket(creator_(key, default_value));

			return *this;
		}

		CString __str__() const
		{
			std::stringstream stream;
			stream << "{";
			for (auto&& kv: *this)
				stream << kv.key << ":" << kv.value << ", ";
			stream << "}";

			return CString(stream.str());
		}
	private:
		// 得到key的hash值在bucket中的索引
		c_size get_hash_index(const Bucket_t& bucket, const K& key) const { return ayrhash(key) % bucket.size(); }


		// 根据key获得存在dict里的KeyValue对象的指针, key不存在返回nullptr
		KV_t* get_kv(const K& key) const
		{
			c_size start_index = get_hash_index(bucket_, key);
			while (bucket_[start_index] != nullptr)
			{
				if (bucket_[start_index]->key_equals(key))
					return bucket_[start_index];

				start_index = (start_index + 1) % bucket_.size();
			}

			return nullptr;
		}

		// 向bucket中添加元素, 并对照更新skip_list
		void setkv2bucket_impl(Bucket_t& bucket, SkipList_t& skip_list, KV_t* kv)
		{
			size_t start_index = get_hash_index(bucket, kv->key);

			Dist_t skipcnt = 0;
			while (bucket[start_index] != nullptr)
			{
				if (skipcnt > skip_list[start_index])
				{
					std::swap(kv, bucket[start_index]);
					std::swap(skipcnt, skip_list[start_index]);
				}

				start_index = (start_index + 1) % bucket.size();
				++skipcnt;
			}

			bucket[start_index] = kv;
			skip_list[start_index] = skipcnt;
		}

		// 向字典中添加元素，不会检查key是否已经存在
		void setkv2bucket(KV_t* kv)
		{
			if (1.0 * size() / bucket_.size() > 0.6) expand(bucket_.size() * 2);

			setkv2bucket_impl(bucket_, skip_list_, kv);

			keys_.append(kv->key);
		}

		void expand(size_t expand_size)
		{
			Bucket_t new_bucket(expand_size, nullptr);
			skip_list_.relloc(expand_size);
			skip_list_.fill(0);

			for (KV_t* kv : bucket_)
				if (kv != nullptr)
					setkv2bucket_impl(new_bucket, skip_list_, kv);

			bucket_ = std::move(new_bucket);
			new_bucket.fill(nullptr);
		}

		virtual self& __iter_container__() const { return const_cast<self&>(*this); }
	private:
		Bucket_t bucket_;

		SkipList_t skip_list_; // 记录每个key离自己原本的位置的距离

		DynArray<K> keys_; // 用于迭代

		C creator_;
	};

	template<Hashable K, typename V, typename Creator>
	class DictGetItem
	{
		using C = Dict<K, V, Creator>;
	public:

		static KeyValue<K, V>& getitem(C& container, size_t index) { return *container.get_kv(container.keys_[index]); }

		static KeyValue<K, V>* getptr(C& container, size_t index) { return container.get_kv(container.keys_[index]); }

		static const KeyValue<K, V>& getcitem(const C& container, size_t index) { return *container.get_kv(container.keys_[index]); }

		static const KeyValue<K, V>* getcptr(const C& container, size_t index) { return container.get_kv(container.keys_[index]); }
	};
}