#pragma once
#include <law/object.hpp>
#include <law/ayr_concepts.hpp>
#include <law/Array.hpp>


namespace ayr
{
	// ����ȡģ����������
	constexpr size_t HASH_PRIME_1 = 1331, HASH_PRIME_2 = 10007;


	template<ayr::DerivedAyr T>
	struct std::hash<T>
	{
		size_t operator()(const T& one) const { return one.__hash__(); }
	};


	// ��ֵ��
	template<Hashable K, typename V>
	struct KeyValue: public Object
	{
		KeyValue(const K& key, const V& value) : key(key), value(value) {}

		KeyValue(K&& key, V&& value) : key(std::move(key)), value(std::move(value)) {}

		KeyValue(const KeyValue& other) : key(other.key), value(other.value) {}

		KeyValue(KeyValue&& other) : key(std::move(other.key)), value(std::move(other.value)) {}
		

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

		size_t key_hash() const { return std::hash<K>()(key); }


		K key;

		V value;
	};


	// ��ϣ�ֵ�
	template<Hashable K, typename V>
	class Dict: public Object
	{
		using Bucket_t = Array<KeyValue<K, V>*>;

		using KV_t = KeyValue<K, V>;

	public:
		Dict(c_size bucket_size): bucket_(bucket_size) {}

		Dict(): Dict(131) {}


	protected:
		// ����hashֵ��Ӧ��Ͱ�±꣬������ж�Ӧ�±걻ռ�ã�����-1
		size_t calc_index(size_t hash_v, const Bucket_t& bucket) const
		{
			size_t index1 = hash_v % HASH_PRIME_1, idnex2 = hash_v % HASH_PRIME_2;
			for (size_t i = 0; i < bucket.size(); ++i)
			{
				size_t index = (index1 + i * idnex2) % bucket.size();
				if (bucket[index] == nullptr)
					return index;
			}

			return -1;
		}

		// �ƶ�Ԫ�ص��µ�Ͱ�����ʧ�ܣ�����false
		bool move_bucket(const Bucket_t& old_bucket, Bucket_t& new_bucket)
		{
			for (auto&& ptr_kv : old_bucket)
			{
				if (ptr_kv == nullptr)
					continue;

				size_t index = calc_index(ptr_kv->key_hash(), new_bucket);

				if (index == -1)
					return false;

				new_bucket[index] = ptr_kv;
			}

			return true;
		}


		// Ͱ����
		void expend_bucket(c_size new_size)
		{
			Bucket_t new_bucket = Bucket_t(new_size, nullptr);

			// û�ҵ�����λ�ã��ݹ�����
			if (!move_bucket(bucket_, new_bucket))
			{
				new_bucket.release();
				expend_bucket(new_size + std::min(131, new_size));
			}
		}


		Bucket_t bucket_;

		c_size size_ = 0;
	};
}