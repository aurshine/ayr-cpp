#pragma once
#include <law/object.hpp>
#include <law/ayr_concepts.hpp>
#include <law/Array.hpp>
#include <law/Chain.hpp>


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
		Dict(c_size bucket_size) : bucket_(bucket_size) {}

		Dict() : Dict(31) {}

		Dict(std::initializer_list<KV_t>&& kv_list)
			: bucket_(kv_list.size() * 3 / 2)
		{
			for (auto&& kv : kv_list)
				this->operator[](kv.key) = kv.value;
		}
		

		V& operator[](const K& key)
		{
			size_t index = get_short_index(key);

			for (auto&& kv : bucket_[index])
				if (kv.key_equals(key))
					return kv.value;

			bucket_[index].prepend(KV_t(key));
			return bucket_[index][0].value;
		}


		const V& operator[](const K& key) const
		{
			size_t index = get_index_from_bucket(bucket_, std::hash<K>()(key));

			for (auto&& kv : bucket_[index])
				if (kv.key_equals(key))
					return kv.value;

			error_assert(false, "KeyError: key not found in dict");
			return None<V>;
		}


		bool contains(const K& key) const
		{
			size_t index = get_index_from_bucket(bucket_, std::hash<K>()(key));

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
		static size_t get_index_from_bucket(const Bucket_t& bucket, size_t hash_v) noexcept
		{
			size_t index = qmi(hash_v % HASH_PRIME_1, hash_v % HASH_PRIME_2, bucket.size());

			return index;
		}


		// 调整桶的大小，使得每个桶的元素个数不超过longest
		size_t get_short_index(const K& key, size_t longest = 7)
		{
			size_t index = 0, hash_v = std::hash<K>()(key);
			while (true)
			{
				index = get_index_from_bucket(bucket_, hash_v);
				if (bucket_[index].size() <= longest)
					break;
				expend();
			}

			return index;
		}

		// 将from中的元素移动到to中
		static void move_bucket(Bucket_t& from, Bucket_t& to) noexcept
		{
			for (auto& chain : from)
				for (auto& kv : chain)
				{
					size_t index = get_index_from_bucket(to, kv.key_hash());
					to[index].prepend(std::move(kv));
				}
		}


		// 扩容
		void expend()
		{
			Bucket_t new_bucket = Bucket_t(std::max(bucket_size() * 3 / 2, bucket_size() + 13));
			move_bucket(bucket_, new_bucket);
			bucket_ = std::move(new_bucket);
		}


		mutable Bucket_t bucket_;
	};
}