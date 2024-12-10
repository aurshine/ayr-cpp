#ifndef AYR_DICT_HPP
#define AYR_DICT_HPP

#include <ranges>

#include "ayr_memory.hpp"
#include "base/bunit.hpp"
#include "base/HashBucket.hpp"

namespace ayr
{
	class Atring;

	// 键值对视图
	template<Hashable K, typename V>
	struct KeyValueView : public Object<KeyValueView<K, V>>
	{
		using self = KeyValueView<K, V>;

		using Key_t = K;

		using Value_t = V;

		KeyValueView() : key_(nullptr), value_(nullptr) {}

		KeyValueView(const Key_t* key, const Value_t* value) : key_(key), value_(value) {}

		KeyValueView(const self& other) = default;

		self& operator=(const self& other) = default;

		self& set_kv(const Key_t* key, const Value_t* value) { key_ = key; value_ = value; return *this; }

		const Key_t& key() const
		{
			if (key_ == nullptr)
				KeyError("key is null");
			return *key_;
		}

		const Value_t& value() const
		{
			if (value_ == nullptr)
				ValueError("value is null");
			return *value_;
		}
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

		Dict(std::initializer_list<std::pair<Key_t, Value_t>>&& kv_list) : Dict(roundup2(kv_list.size() / MAX_LOAD_FACTOR))
		{
			for (auto&& kv : kv_list)
				insert(kv.first, kv.second);
		}

		Dict(const self& other) : bucket_(other.bucket_), keys_(other.keys_) {};

		Dict(self&& other)noexcept : bucket_(std::move(other.bucket_)), keys_(std::move(other.keys_)) {};

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

		~Dict() = default;

		// key-value对的数量
		c_size size() const { return keys_.size(); }

		// 桶容量
		c_size capacity() const { return bucket_.capacity(); }

		bool contains(const Key_t& key) const { return contains_hashv(ayrhash(key)); }

		// 负载因子
		double load_factor() const { return 1.0 * size() / capacity(); }

		// 键的迭代视图
		auto keys() const { return std::ranges::subrange(keys_.begin(), keys_.end()); }

		// 值的迭代视图
		auto values() const { return std::ranges::subrange(bucket_.begin(), bucket_.end()); }

		// 重载[]运算符, key 必须存在, 否则KeyError
		const V& operator[](const Key_t& key) const { return get(key); }

		// 重载[]运算符, 若key不存在, 则创建并返回一个默认值
		Value_t& operator[](const Key_t& key)
		{
			hash_t hashv = ayrhash(key);
			if (!contains_hashv(hashv))
				return insert_impl(key, V(), hashv);

			return *get_impl(hashv);
		}

		// 获得key对应的value, 若key不存在, 则抛出KeyError
		Value_t& get(const Key_t& key)
		{
			Value_t* value = get_impl(ayrhash(key));

			if (value != nullptr) return *value;
			KeyError(std::format("Key '{}' not found in dict", key));
			return None<V>;
		}

		// 获得key对应的value, 若key不存在, 则抛出KeyError
		const Value_t& get(const Key_t& key) const
		{
			const Value_t* value = get_impl(ayrhash(key));

			if (value != nullptr) return *value;
			KeyError(std::format("Key '{}' not found in dict", key));
			return None<V>;
		}

		// 获得key对应的value, 若key不存在, 则返回default_value
		Value_t& get(const Key_t& key, Value_t& default_value)
		{
			Value_t* value = get_impl(ayrhash(key));
			if (value != nullptr) return *value;

			return default_value;
		}

		// 获得key对应的value, 若key不存在, 则返回default_value
		const Value_t& get(const Key_t& key, const Key_t& default_value) const
		{
			const Value_t* value = get_impl(ayrhash(key));
			if (value != nullptr) return *value;

			return default_value;
		}

		// 若key不存在, 则添加一个默认值
		template<typename _V>
		self& setdefault(const Key_t& key, _V&& default_value)
		{
			hash_t hashv = ayrhash(key);
			if (!contains_hashv(hashv))
				insert_impl(key, std::forward<_V>(default_value), hashv);

			return *this;
		}

		// 根据传入的字典更新字典
		self& update(const self& other)
		{
			for (auto&& kv : other)
				insert(kv.key(), kv.value());

			return *this;
		}

		self& update(self&& other)
		{
			for (auto& key : other.keys_)
			{
				hash_t hashv = ayrhash(key);
				Value_t& value = *other.get_impl(hashv);
				insert(std::move(key), std::move(value), hashv);
			}
			return *this;
		}

		// 向字典中插入一个key-value对, 若key已经存在, 则覆盖原有值
		template<typename _K, typename _V>
		Value_t& insert(_K&& key, _V&& value)
		{
			hash_t hashv = ayrhash(key);
			Value_t* m_value = get_impl(hashv);
			if (m_value != nullptr)
				return *m_value = std::forward<_V>(value);

			return insert_impl(std::forward<_K>(key), std::forward<_V>(value), hashv);
		}

		template<typename _K, typename _V>
		Value_t& insert(_K&& key, _V&& value, hash_t hashv)
		{
			Value_t* m_value = get_impl(hashv);
			if (m_value != nullptr)
				return *m_value = std::forward<_V>(value);

			return insert_impl(std::forward<_K>(key), std::forward<_V>(value), hashv);
		}

		void pop(const Key_t& key)
		{
			hash_t hashv = ayrhash(key);
			bucket_.pop(hashv);
			keys_.pop(keys_.find(key));
		}

		void clear() { bucket_.clear(); keys_.clear(); }

		self operator&(const self& other) const
		{
			if (this == &other) return *this;
			else if (size() > other.size()) return other & *this;

			self result((std::min)(capacity(), other.capacity()));
			for (auto&& kv : *this)
				if (other.contains(kv.key()))
					result.insert_impl(kv.key(), kv.value(), ayrhash(kv.key()));

			return result;
		}

		self& operator&= (const self& other)
		{
			if (this == &other) return *this;
			*this = *this & other;
			return *this;
		}

		self operator|(const self& other) const
		{
			if (this == &other) return *this;
			else if (size() > other.size()) return other | *this;

			self result(static_cast<c_size>((size() + other.size()) / MAX_LOAD_FACTOR));
			for (auto&& kv : *this)
				result.insert_impl(kv.key(), kv.value(), ayrhash(kv.key()));

			for (auto&& kv : other)
				result.insert(kv.key(), kv.value());

			return result;
		}

		self& operator|= (const self& other)
		{
			if (this == &other) return *this;

			for (auto&& kv : other)
				insert(kv.key(), kv.value());

			return *this;
		}

		self operator^(const self& other) const
		{
			if (this == &other) return self();
			else if (size() > other.size()) return other ^ *this;

			self result((std::min)(capacity(), other.capacity()));
			for (auto&& kv : *this)
				if (!other.contains(kv.key()))
					result.insert_impl(kv.key(), kv.value(), ayrhash(kv.key()));

			for (auto&& kv : other)
				if (!contains(kv.key()))
					result.insert_impl(kv.key(), kv.value(), ayrhash(kv.key()));
			return result;
		}

		self& operator^= (const self& other)
		{
			if (this == &other)
			{
				clear();
				return *this;
			}

			for (auto&& kv : other)
				if (contains(kv.key()))
					pop(kv.key());
				else
					insert_impl(kv.key(), kv.value(), ayrhash(kv.key()));

			return *this;
		}


		self operator+ (const self& other) const
		{
			return *this | other;
		}

		self& operator+= (const self& other)
		{
			return *this |= other;
		}

		self operator- (const self& other) const
		{
			if (this == &other) return self();

			self result(capacity());
			for (auto&& kv : *this)
				if (!other.contains(kv.key()))
					result.insert_impl(kv.key(), kv.value(), ayrhash(kv.key()));

			return result;
		}

		self& operator-= (const self& other)
		{
			if (this == &other)
			{
				clear();
				return *this;
			}

			for (auto&& kv : other)
				pop(kv.key());

			return *this;
		}

		bool __equals__(const self& other) const
		{
			if (size() != other.size()) return false;
			for (auto&& kv : *this)
			{
				if (!other.contains(kv.key()))
					return false;

				if (kv.value() != other.get(kv.key()))
					return false;
			}
			return true;
		}

		CString __str__() const
		{
			std::stringstream stream;
			stream << "{";
			for (auto&& item : *this)
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

			bool __equals__(const self& other) const { return dict_ == other.dict_ && index_ == other.index_; }

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

		void expand() { bucket_.expand(std::max<c_size>(capacity() * 2, MIN_BUCKET_SIZE)); }

		// 向字典中插入一个key-value对
		// 不检查key是否存在
		template<typename _K, typename _V>
		Value_t& insert_impl(_K&& key, _V&& value)
		{
			insert_impl(std::forward<_K>(key), std::forward<_V>(value), ayrhash(static_cast<const Key_t&>(key)));
		}

		template<typename _K, typename _V>
		Value_t& insert_impl(_K&& key, _V&& value, hash_t hashv)
		{
			if (load_factor() >= MAX_LOAD_FACTOR)
				expand();

			Value_t& v = *bucket_.set_value(std::forward<_V>(value), hashv);
			keys_.append(std::forward<_K>(key));

			return v;
		}

		// 获得hash值对应的value指针
		// 不检查key是否存在
		Value_t* get_impl(hash_t hashv) { return bucket_.try_get(hashv); }

		const Value_t* get_impl(hash_t hashv) const { return bucket_.try_get(hashv); }
	private:
		Bucket_t bucket_;

		DynArray<Key_t> keys_;

		static constexpr double MAX_LOAD_FACTOR = 0.75;

		static constexpr c_size MIN_BUCKET_SIZE = 8;
	};
}
#endif