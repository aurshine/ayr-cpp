#ifndef AYR_LAW_DICT_HPP
#define AYR_LAW_DICT_HPP

#include <ranges>

#include <ayr/ayr_memory.hpp>
#include <ayr/detail/bunit.hpp>
#include <ayr/detail/HashBucket.hpp>
#include <ayr/detail/RelationIterator.hpp>
#include <ayr/Atring.hpp>

namespace ayr
{
	// 键值对
	template<Hashable K, typename V>
	struct KeyValueView : public Object<KeyValueView<K, V>>
	{
		using self = KeyValueView<K, V>;

		using Key_t = K;

		using Value_t = V;

		KeyValueView() : key_(nullptr), value_(nullptr) {}

		KeyValueView(const Key_t* key, const Value_t* value) : key_(key), value_(value) {}

		KeyValueView(const self& other) : key_(other.key_), value_(other.value_) {}

		self& operator=(const self& other)
		{
			key_ = other.key_;
			value_ = other.value_;
			return *this;
		}

		self& set_kv(const Key_t* key, const Value_t* value)
		{
			key_ = key;
			value_ = value;
			return *this;
		}

		const Key_t& key() const { error_assert(key_ != nullptr, "key is null"); return *key_; }

		const Value_t& value() const { error_assert(value_ != nullptr, "value is null"); return *value_; }
	private:
		const Key_t* key_;

		const Value_t* value_;
	};


	template<Hashable K, typename V, typename Bucket_t = RobinHashBucket<V>>
	class Dict : public Object<Dict<K, V>>
	{
		using self = Dict<K, V>;
	public:
		using Key_t = K;

		using Value_t = V;

		Dict(c_size bucket_size = MIN_BUCKET_SIZE) : bucket_(bucket_size), keys_() {}

		Dict(std::initializer_list<std::pair<Key_t, Value_t>>&& kv_list) : Dict(roundup2(c_size(kv_list.size() / MAX_LOAD_FACTOR)))
		{
			for (auto&& kv : kv_list)
				insert_impl(std::move(kv.first), std::move(kv.second), ayrhash(kv.first));
		}

		Dict(const self& other) : bucket_(other.bucket_), keys_(other.keys_) {}

		Dict(self&& other) noexcept : bucket_(std::move(other.bucket_)), keys_(std::move(other.keys_)) {}

		Dict& operator=(const self& other)
		{
			if (this == &other) return *this;
			bucket_ = other.bucket_;
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

		c_size capacity() const { return bucket_.capacity(); }

		bool contains(const K& key) const { return contains_hashv(ayrhash(key)); }

		double load_factor() const { return 1.0 * size() / capacity(); }

		auto keys() { return std::ranges::subrange(keys_.begin(), keys_.end()); }

		auto keys() const { return std::ranges::subrange(keys_.begin(), keys_.end()); }

		auto values() { return std::ranges::subrange(bucket_.begin(), bucket_.end()); }

		auto values() const { return std::ranges::subrange(bucket_.begin(), bucket_.end()); }

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
			Value_t* value = bucket_.try_get(hashv);

			if (value != nullptr) return *value;
			KeyError("Key not found in dict");
			return None<V>;
		}

		// 获得key对应的value, 若key不存在, 则抛出KeyError
		const V& get(const K& key) const
		{
			const Value_t* value = bucket_.try_get(ayrhash(key));

			if (value != nullptr) return *value;
			KeyError("Key not found in dict");
			return None<V>;
		}

		// 获得key对应的value, 若key不存在, 则返回default_value
		V& get(const K& key, V& default_value)
		{
			Value_t* value = bucket_.try_get(ayrhash(key));
			if (value != nullptr) return *value;

			return default_value;
		}

		// 获得key对应的value, 若key不存在, 则返回default_value
		const V& get(const K& key, const V& default_value) const
		{
			const Value_t* value = bucket_.try_get(ayrhash(key));
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
			Value_t* m_value = bucket_.try_get(hashv);
			if (m_value != nullptr)
				*m_value = value;
			else
				insert_impl(key, value, hashv);
		}

		void insert(K&& key, V&& value)
		{
			hash_t hashv = ayrhash(key);
			Value_t* m_value = bucket_.try_get(hashv);
			if (m_value != nullptr)
				*m_value = std::move(value);
			else
				insert_impl(std::move(key), std::move(value), hashv);
		}

		void clear() { bucket_.clear(); keys_.clear(); }

		CString __str__() const override
		{
			std::stringstream stream;
			stream << "{";
			for (auto& item : *this)
			{
				if constexpr (issame<Key_t, CString, Atring, std::string>)
					stream << "\"" << item.key() << "\"";
				else
					stream << item.key();
				stream << ": ";
				if constexpr (issame<Value_t, CString, Atring, std::string>)
					stream << "\"" << item.value() << "\"";
				else
					stream << item.value();
				stream << ", ";
			}

			std::string str = stream.str();
			if (str.size() > 2)
			{
				str.pop_back();
				str.pop_back();
			}
			str.push_back('}');

			return str;
		}

		class DictIterator : public Object<DictIterator>
		{
			using self = DictIterator;

			using super = Object<self>;
		public:
			using iterator_category = std::random_access_iterator_tag;

			using value_type = KeyValueView<Key_t, Value_t>;

			using difference_type = std::ptrdiff_t;

			using pointer = value_type*;

			using const_pointer = const value_type*;

			using reference = value_type&;

			using const_reference = const value_type&;

			DictIterator() : dict_(nullptr), index_(0), kv_() {}

			DictIterator(const Dict* dict, c_size index) : dict_(dict), index_(index) { update_kv(0); }

			DictIterator(const self& other) = default;

			self& operator=(const self& other) = default;

			const value_type& operator*() const { return kv_; }

			const value_type* operator->() const { return &kv_; }

			self& operator++() { update_kv(1); return *this; }

			self operator++(int) { self tmp = *this; update_kv(1); return tmp; }

			self& operator--() { update_kv(-1); return *this; }

			self operator--(int) { self tmp = *this; update_kv(-1); return tmp; }

			self& operator+=(difference_type n) { update_kv(n); return *this; }

			self& operator-=(difference_type n) { update_kv(-n); return *this; }

			self operator+(difference_type n) const { return self(dict_, index_ + n); }

			self operator-(difference_type n) const { return self(dict_, index_ - n); }

			difference_type operator-(const self& other) const { return index_ - other.index_; }

			bool __equals__(const self& other) const override { return dict_ == other.dict_ && index_ == other.index_; }

		private:
			void update_kv(c_size add)
			{
				index_ += add;
				if (index_ < 0 || index_ >= dict_->size())
					kv_.set_kv(nullptr, nullptr);
				else
				{
					const Key_t& key = dict_->keys_[index_];
					kv_.set_kv(&key, &dict_->get(key));
				}
			}
		private:
			const Dict* dict_;

			c_size index_;

			value_type kv_;
		};

		using Iterator = DictIterator;

		using ConstIterator = DictIterator;

		Iterator begin() { return Iterator(this, 0); }

		Iterator end() { return Iterator(this, size()); }

		ConstIterator begin() const { return ConstIterator(this, 0); }

		ConstIterator end() const { return ConstIterator(this, size()); }
	private:
		bool contains_hashv(hash_t hashv) const { return bucket_.contains(hashv); }

		void expand() { bucket_.expand(std::max(capacity() * 2, MIN_BUCKET_SIZE)); }

		// 插入一个key-value对, 不对key是否存在做检查
		void insert_impl(const K& key, const V& value, hash_t hashv)
		{
			if (load_factor() >= MAX_LOAD_FACTOR)
				expand();

			bucket_.set_store(value, hashv);
			keys_.append(key);
		}

		void insert_impl(K&& key, V&& value, hash_t hashv)
		{
			if (load_factor() >= MAX_LOAD_FACTOR)
				expand();

			bucket_.set_store(std::move(value), hashv);
			keys_.append(std::move(key));
		}
	private:
		Bucket_t bucket_;

		DynArray<Key_t> keys_;

		static constexpr double MAX_LOAD_FACTOR = 0.75;

		static constexpr c_size MIN_BUCKET_SIZE = 8;
	};
}
#endif