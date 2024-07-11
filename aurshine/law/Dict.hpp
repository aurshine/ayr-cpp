#pragma once
#include <law/object.hpp>
#include <law/hash.hpp>
#include <law/Array.hpp>


namespace ayr
{
	// 键值对
	template<Hashable K, typename V>
	struct KeyValue : public Object
	{
		KeyValue() {};

		KeyValue(const K& key) : key(key) {}

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


	// 哈希字典
	template<Hashable K, typename V, typename C = Creator<KeyValue<K, V>>>
	class Dict : public Object
	{
		using KV_t = KeyValue<K, V>;

	public:
		Dict(c_size bucket_size) : bucket_(bucket_size, nullptr), size_(0), hasher_() {}

		Dict() : Dict(31) {}

		Dict(std::initializer_list<KV_t>&& kv_list)
			: bucket_(kv_list.size() / 0.7)
		{
			for (auto&& kv : kv_list)
				this->operator[](kv.key) = kv.value;
		}


		V& operator[](const K& key)
		{
			return None<V>;
		}


		const V& operator[](const K& key) const
		{
			return None<V>;
		}


		bool contains(const K& key) const
		{

		}


		Dict setdefault(const K& key, const V& default_value)
		{
			if (!contains(key))
				this->operator[](key) = default_value;

			return *this;
		}

	private:
		c_size get_hash_index(const K& key) const { return hash(key) % bucket_.size(); }


	private:
		Array<KV_t*> bucket_;

		Array<uint8_t> skip_list_; // 记录每个key离自己原本的位置的距离

		size_t size_;

		C creator_;
	};
}