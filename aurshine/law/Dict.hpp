#ifndef AYR_LAW_DICT_HPP
#define AYR_LAW_DICT_HPP

#include <law/ayr_memory.hpp>
#include <law/detail/bunit.hpp>
#include <law/detail/hash_bucket.hpp>
#include <law/detail/RelationIterator.hpp>

namespace ayr
{
	// 键值对
	template<Hashable K, typename V>
	struct KeyValue : public Object<KeyValue<K, V>>
	{
		using self = KeyValue<K, V>;

		KeyValue() : key(), value() {}

		KeyValue(const K& key, const V& value) : key(key), value(value) {}

		KeyValue(std::remove_reference_t<K>&& key, std::remove_reference_t<V>&& value) : key(std::move(key)), value(std::move(value)) {}

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


	template<Hashable K>
	struct KeyValue<K, void> : public Object<KeyValue<K, void>>
	{
		using self = KeyValue<K, void>;

		KeyValue() : key() {}

		KeyValue(const K& key) : key(key) {}

		KeyValue(K&& key) : key(std::move(key)) {}

		KeyValue(const self& other) : key(other.key) {}

		KeyValue(self&& other) : key(std::move(other.key)) {}

		size_t key_hash() const { return ayrhash(key); }

		bool key_equals(const K& other) const { return key == other; }

		self& operator=(const self& other)
		{
			if (this == &other) return *this;
			key = other.key;
			return *this;
		}

		self& operator=(self&& other) noexcept
		{
			if (this == &other) return *this;
			key = std::move(other.key);
			return *this;
		}

		K key;
	};


	template<Hashable K, typename V>
	class Dict : public Object<Dict<K, V>>
	{
		using self = Dict<K, V>;
	public:
		using Key_t = K;

		using Value_t = V;

		using KV_t = KeyValue<Key_t, Value_t>;

		using Bucket_t = HashBucketImpl<Value_t>;

		Dict(c_size bucket_size = MIN_BUCKET_SIZE) : bucket_(std::make_unique<RobinHashBucket<Value_t>>(bucket_size)), keys_() {}

		Dict(Bucket_t* bucket) : bucket_(bucket), keys_() {}

		Dict(std::initializer_list<KV_t>&& kv_list) : Dict(roundup2(c_size(kv_list.size() / MAX_LOAD_FACTOR)))
		{
			for (auto&& kv : kv_list)
				insert_impl(std::move(kv.key), std::move(kv.value));
		}

		Dict(const self& other) : bucket_(other.bucket_->clone()), keys_(other.keys_) {}

		Dict(self&& other) noexcept : bucket_(std::move(other.bucket_)), keys_(std::move(other.keys_)) {}

		~Dict() {}

		Dict& operator=(const self& other)
		{
			if (this == &other) return *this;
			bucket_ = other.bucket_->clone();
			keys_ = other.keys_;
			return *this;
		}

		Dict& operator=(self&& other) noexcept
		{
			if (this == &other) return *this;
			bucket_ = std::move(other.bucket_);
			keys_ = std::move(other.keys_);
			return *this;
		}

		// key-value对的数量
		c_size size() const { return keys_.size(); }

		c_size capacity() const { return bucket_->capacity(); }

		bool contains(const K& key) const { return contains_hashv(ayrhash(key)); }

		double load_factor() const { return 1.0 * size() / capacity(); }

		DynArray<Key_t>& keys() { return keys_; }

		const DynArray<Key_t>& keys() const { return keys_; }

		// 重载[]运算符, key 必须存在, 否则KeyError
		const V& operator[](const K& key) const { return get(key); }

		// 重载[]运算符, 若key不存在, 则创建并返回一个默认值
		V& operator[](const K& key)
		{
			hash_t hashv = ayrhash(key);
			if (!contains_hashv(hashv))
				insert(key, V());

			return get(key);
		}

		// 获得key对应的value, 若key不存在, 则抛出KeyError
		V& get(const K& key)
		{
			hash_t hashv = ayrhash(key);
			Value_t* value = bucket_->try_get(hashv);

			if (value != nullptr) return *value;
			KeyError("Key not found in dict");
			return None<V>;
		}

		// 获得key对应的value, 若key不存在, 则抛出KeyError
		const V& get(const K& key) const
		{
			Value_t* value = bucket_->try_get(ayrhash(key));

			if (value != nullptr) return *value;
			KeyError("Key not found in dict");
			return None<V>;
		}

		// 获得key对应的value, 若key不存在, 则返回default_value
		V& get(const K& key, V& default_value)
		{
			Value_t* value = bucket_->try_get(ayrhash(key));
			if (value != nullptr) return *value;

			return default_value;
		}

		// 获得key对应的value, 若key不存在, 则返回default_value
		const V& get(const K& key, const V& default_value) const
		{
			Value_t* value = bucket_->try_get(ayrhash(key));
			if (value != nullptr) return *value;

			return default_value;
		}

		// 若key不存在, 则添加一个默认值
		self& setdefault(const K& key, const V& default_value)
		{
			if (!contains(key)) insert_impl(key, default_value);

			return *this;
		}

		// 根据传入的字典更新字典
		self& update(const self& other)
		{
			for (auto&& kv : other)
				insert(kv.key, kv.value);

			return *this;
		}

		// 向字典中插入一个key-value对, 若key已经存在, 则覆盖原有值
		void insert(const K& key, const V& value)
		{
			hash_t hashv = ayrhash(key);
			Value_t* m_value = bucket_->try_get(hashv);
			if (m_value != nullptr)
				*m_value = value;
			else
				insert_impl(key, value, hashv);
		}

		void insert(K&& key, V&& value)
		{
			hash_t hashv = ayrhash(key);
			Value_t* m_value = bucket_->try_get(hashv);
			if (m_value != nullptr)
				*m_value = std::move(value);
			else
				insert_impl(std::move(key), std::move(value), hashv);
		}

		void clear() { bucket_->clear(); keys_.clear(); }

		CString __str__() const override
		{
			std::stringstream stream;
			stream << "{";
			for (int i = 0, key_size = keys_.size(); i < key_size; i++)
			{
				auto&& key = keys_[i];
				if (i) stream << ", ";
				if constexpr (issame<K, CString>)
					stream << "\"" << key << "\": " << get(key);
				else
					stream << key << ": " << get(key);
			}

			stream << "}";

			return stream.str();
		}
	private:
		bool contains_hashv(hash_t hashv) const { return bucket_->contains(hashv); }

		void expand() { bucket_->expand(std::max(capacity() * 2, MIN_BUCKET_SIZE)); }

		// 插入一个key-value对, 不对key是否存在做检查
		void insert_impl(const K& key, const V& value, hash_t hashv = ayrhash(key))
		{
			if (load_factor() >= MAX_LOAD_FACTOR)
				expand();

			bucket_->set_store(value, hashv);
			keys_.append(key);
		}

		void insert_impl(K&& key, V&& value, hash_t hashv = ayrhash(key))
		{
			if (load_factor() >= MAX_LOAD_FACTOR)
				expand();

			bucket_->set_store(std::move(value), hashv);
			keys_.append(std::move(key));
		}
	private:
		std::unique_ptr<Bucket_t> bucket_;

		DynArray<Key_t> keys_;

		static constexpr double MAX_LOAD_FACTOR = 0.75;

		static constexpr c_size MIN_BUCKET_SIZE = 8;
	};
}
#endif