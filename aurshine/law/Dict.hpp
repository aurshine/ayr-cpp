#pragma once
#include <law/object.hpp>
#include <law/ayr_concepts.hpp>s
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


	inline constexpr uint64_t decode_fixed32(const char* ptr)
	{
		return ((static_cast<uint64_t>(static_cast<uint8_t>(ptr[0]))) |
			(static_cast<uint64_t>(static_cast<uint8_t>(ptr[1])) << 8) |
			(static_cast<uint64_t>(static_cast<uint8_t>(ptr[2])) << 16) |
			(static_cast<uint64_t>(static_cast<uint8_t>(ptr[3])) << 24));
	}


	inline constexpr hash_t bytes_hash(const char* data, size_t n, uint32_t seed)
	{
		constexpr hash_t m = 0xc6a4a793;
		constexpr hash_t r = 24;
		const char* end = data + n;
		hash_t h = seed ^ (n * m);

		while (data < end)
		{
			hash_t w = decode_fixed32(data);
			data += 4;
			h = (h + w) * m;
			h ^= (h >> 16);
		}

		int dis = end - data;
		while (dis--)
		{
			h += static_cast<uint8_t>(data[dis - 1] << (dis - 1) * 8);
		}
		h *= m;
		h ^= (h >> r);
		return h;
	}


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
	template<Hashable K, typename V>
	class Dict : public Object
	{
		using KV_t = KeyValue<K, V>;

		using Bucket_t = Array<BiChain<KV_t>>;

	public:
		Dict(c_size bucket_size) : bucket_(bucket_size), size_(0), hasher_() {}

		Dict() : Dict(31) {}

		Dict(std::initializer_list<KV_t>&& kv_list)
			: bucket_(kv_list.size() / 0.7)
		{
			for (auto&& kv : kv_list)
				this->operator[](kv.key) = kv.value;
		}


		V& operator[](const K& key)
		{
			if (load_factor() >= 0.7)
				expend();

			size_t index = get_index_from_bucket(bucket_, hasher_(key));

			for (auto&& kv : bucket_[index])
				if (kv.key_equals(key))
					return kv.value;

			bucket_[index].prepend(KV_t(key));
			++size_;
			return bucket_[index][0].value;
		}


		const V& operator[](const K& key) const
		{
			size_t index = get_index_from_bucket(bucket_, hasher_(key));

			for (auto&& kv : bucket_[index])
				if (kv.key_equals(key))
					return kv.value;

			error_assert(false, "KeyError: key not found in dict");
			return None<V>;
		}


		bool contains(const K& key) const
		{
			size_t index = get_index_from_bucket(bucket_, hasher_(key));

			for (auto&& kv : bucket_[index])
				if (kv.key_equals(key))
					return true;

			return false;
		}


		Dict setdefault(const K& key, const V& default_value)
		{
			if (!contains(key))
				this->operator[](key) = default_value;

			return *this;
		}


		c_size bucket_size() const { return bucket_.size(); }

	private:
		// 通过哈希值获取对应桶的索引
		size_t get_index_from_bucket(const Bucket_t& bucket, size_t hash_v) noexcept
		{
			size_t index = ((hash_v % HASH_PRIME_1) ^ (hash_v % HASH_PRIME_2) << 8) % bucket.size();

			return index;
		}


		// 将from中的元素移动到to中
		void move_bucket(Bucket_t& from, Bucket_t& to) noexcept
		{
			for (auto& chain : from)
				for (auto& kv : chain)
				{
					size_t index = get_index_from_bucket(to, hasher_(kv.key));
					to[index].prepend(std::move(kv));
				}
		}

		double load_factor() const { return size_ / (double)bucket_.size(); }

		// 扩容
		void expend()
		{
			Bucket_t new_bucket = Bucket_t(bucket_size() * 2);
			move_bucket(bucket_, new_bucket);
			bucket_ = std::move(new_bucket);
		}

		c_size size_;

		mutable Bucket_t bucket_;

		std::hash<K> hasher_;
	};
}