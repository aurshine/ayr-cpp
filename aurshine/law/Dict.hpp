#pragma once
#include <law/object.hpp>
#include <law/ayr_concepts.hpp>
#include <law/Array.hpp>
#include <law/Chain.hpp>


namespace ayr
{
	// 用于取模的质数常数
	constexpr size_t HASH_PRIME_1 = 131, HASH_PRIME_2 = 1007;


	template<ayr::DerivedAyr T>
	struct std::hash<T>
	{
		size_t operator()(const T& one) const { return one.__hash__(); }
	};


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

		int bias = 0;
	};


	// 哈希字典
	template<Hashable K, typename V, typename C = Creator<KeyValue<K, V>>>
	class Dict : public Object
	{
		using KV_t = KeyValue<K, V>;

	public:
		Dict(c_size bucket_size) : bucket_(bucket_size, 0), size_(0), hasher_() {}

		Dict() : Dict(31) {}

		Dict(std::initializer_list<KV_t>&& kv_list)
			: bucket_(kv_list.size() / 0.7)
		{
			for (auto&& kv : kv_list)
				this->operator[](kv.key) = kv.value;
		}


		V& operator[](const K& key)
		{

		}


		const V& operator[](const K& key) const
		{

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
		c_size get_hash_index(const K& key) const { return hasher_(key) % bucket_.size(); }


	private:
		Array<KV_t*> bucket_;

		size_t size_;

		std::hash<K> hasher_;

		C creator_;
	};
}