#ifndef AYR_DICT_HPP
#define AYR_DICT_HPP

#include <ranges>

#include "ayr_memory.hpp"
#include "Chain.hpp"
#include "base/HashBucket.hpp"
#include "base/View.hpp"

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

		KeyValueView() : key_(), value_() {}

		KeyValueView(Key_t& key, Value_t& value) : key_(key), value_(value) {}

		KeyValueView(const self& other) : key_(other.key_), value_(other.value_) {};

		self& operator=(const self& other)
		{
			if (this == &other) return *this;

			key_ = other.key_;
			value_ = other.value_;
			return *this;
		}

		self& set_kv(Key_t& key, Value_t& value) { key_ = key; value_ = value; return *this; }

		Key_t& key() { return key_; }

		Value_t& value() { return value_; }

		const Key_t& key() const { return key_; }

		const Value_t& value() const { return value_; }

		hash_t hashv() const { return ayrhash(key_.get<Key_t>()); }
	private:
		View key_, value_;
	};

	template<Hashable K, typename V, typename Bucket_t = RobinHashBucket<V>>
	class Dict : public Object<Dict<K, V>>
	{
		using self = Dict<K, V>;
	public:
		using Key_t = K;

		using Value_t = V;

		using Iterator = BiChain<Key_t>::Iterator;

		using ConstIterator = BiChain<Key_t>::ConstIterator;

		template<bool IsConst>
		struct KeyValueViewIterator : public IteratorInfo<
			KeyValueViewIterator<IsConst>,
			add_const_t<IsConst, Dict>,
			typename DynArray<Key_t>::Iterator::iterator_category,
			KeyValueView<add_const_t<IsConst, Key_t>, Value_t>
		>
		{
			using ItInfo = IteratorInfo<
				KeyValueViewIterator<IsConst>,
				add_const_t<IsConst, Dict>,
				typename DynArray<Key_t>::Iterator::iterator_category,
				KeyValueView<add_const_t<IsConst, Key_t>, add_const_t<IsConst, Value_t>>
			>;

			using DI = std::conditional_t<IsConst, typename BiChain<Key_t>::ConstIterator, typename BiChain<Key_t>::Iterator>;

			KeyValueViewIterator() : dict_(nullptr), it_(), kv_() {}

			KeyValueViewIterator(ItInfo::container_type* dict, DI it) : dict_(dict), it_(it), kv_() {}

			KeyValueViewIterator(const KeyValueViewIterator& other) : dict_(other.dict_), it_(other.it_), kv_(other.kv_) {}

			KeyValueViewIterator& operator=(const KeyValueViewIterator& other)
			{
				if (this == &other) return *this;

				dict_ = other.dict_;
				it_ = other.it_;
				kv_ = other.kv_;
				return *this;
			}

			typename ItInfo::reference operator*() const { update_kv(); return kv_; }

			typename ItInfo::pointer operator->() const { update_kv(); return &kv_; }

			typename ItInfo::iterator_type& operator++() { ++it_; return *this; }

			typename ItInfo::iterator_type operator++(int) { auto tmp = *this; ++(*this); return tmp; }

			typename ItInfo::iterator_type& operator--() { --it_; return *this; }

			typename ItInfo::iterator_type operator--(int) { auto tmp = *this; --(*this); return tmp; }

			typename ItInfo::iterator_type& operator+=(typename ItInfo::difference_type n) { it_ += n; return *this; }

			typename ItInfo::iterator_type operator+(typename ItInfo::difference_type n) const { auto tmp = *this; tmp += n; return tmp; }

			typename ItInfo::iterator_type& operator-=(typename ItInfo::difference_type n) { it_ -= n; return *this; }

			typename ItInfo::iterator_type operator-(typename ItInfo::difference_type n) const { auto tmp = *this; tmp -= n; return tmp; }

			typename ItInfo::difference_type operator-(const typename ItInfo::iterator_type& other) const { return it_ - other.it_; }

			bool __equals__(const typename ItInfo::iterator_type& other) const { return it_ == other.it_; }
		private:
			void update_kv() const { kv_.set_kv(*it_, dict_->get(*it_)); }

			typename ItInfo::container_type* dict_;

			mutable typename ItInfo::value_type kv_;

			DI it_;
		};

		using KvIterator = KeyValueViewIterator<false>;

		using ConstKvIterator = KeyValueViewIterator<true>;

		Dict() : bucket_(MIN_BUCKET_SIZE), keys_() {}

		Dict(c_size size) : bucket_(std::max(MIN_BUCKET_SIZE, adapt_bucket_size(size, MAX_LOAD_FACTOR))), keys_() {}

		Dict(std::initializer_list<std::pair<Key_t, Value_t>>&& kv_list) : Dict(adapt_bucket_size(kv_list.size(), MAX_LOAD_FACTOR))
		{
			for (auto&& kv : kv_list)
				insert(kv.first, kv.second);
		}

		Dict(const self& other) : bucket_(other.bucket_), keys_(other.keys_) {}

		Dict(self&& other) noexcept : bucket_(std::move(other.bucket_)), keys_(std::move(other.keys_)) {}

		Dict& operator=(const self& other)
		{
			if (this == &other) return *this;

			return *ayr_construct(this, other);
		}

		Dict& operator=(self&& other) noexcept
		{
			if (this == &other) return *this;

			return *ayr_construct(this, std::move(other));
		}

		// key-value对的数量
		c_size size() const { return keys_.size(); }

		// 桶容量
		c_size capacity() const { return bucket_.capacity(); }

		bool contains(const Key_t& key) const { return contains_hashv(ayrhash(key)); }

		// 负载因子
		double load_factor() const { return 1.0 * size() / capacity(); }

		// 键的迭代视图
		auto keys() const { return std::ranges::subrange(begin(), end()); }

		// 值的迭代视图
		auto values() const { return std::ranges::subrange(bucket_.begin(), bucket_.end()); }

		auto items() { return std::ranges::subrange(KvIterator(this, begin()), KvIterator(this, end())); }

		auto items() const { return std::ranges::subrange(ConstKvIterator(this, begin()), ConstKvIterator(this, end())); }

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
			for (auto&& item : other.items())
				insert(item.key(), item.value());

			return *this;
		}

		self& update(self&& other)
		{
			for (auto&& item : other.items())
				insert(std::move(item.key()), std::move(item.value()), item.hashv());

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

		/*void pop(const Key_t& key)
		{
			hash_t hashv = ayrhash(key);
			bucket_.pop(hashv);
			keys_.pop(keys_.find(key));
		}*/

		void clear() { bucket_.clear(); keys_.clear(); }

		self operator&(const self& other) const
		{
			if (this == &other) return *this;
			else if (size() > other.size()) return other & *this;

			self result((std::min)(capacity(), other.capacity()));
			for (auto&& item : items())
				if (other.contains(item.key()))
					result.insert_impl(item.key(), item.value(), item.hashv());

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
			for (auto&& item : items())
				result.insert_impl(item.key(), item.value(), item.hashv());

			for (auto&& item : other.items())
				result.insert(item.key(), item.value());

			return result;
		}

		self& operator|= (const self& other)
		{
			if (this == &other) return *this;

			for (auto&& item : other.items())
				insert(item.key(), item.value());

			return *this;
		}

		self operator^(const self& other) const
		{
			if (this == &other) return self();
			else if (size() > other.size()) return other ^ *this;

			self result(std::min(capacity(), other.capacity()));
			for (auto&& item : items())
				if (!other.contains(item.key()))
					result.insert_impl(item.key(), item.value(), item.hashv());

			for (auto&& item : other.items())
				if (!contains(item.key()))
					result.insert_impl(item.key(), item.value(), item.hashv());
			return result;
		}

		self& operator^= (const self& other)
		{
			if (this == &other)
			{
				clear();
				return *this;
			}

			for (auto&& item : other.items())
				if (contains(item.key()))
					pop(item.key());
				else
					insert_impl(item.key(), item.value(), item.hashv());

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
			for (auto&& item : items())
				if (!other.contains(item.key()))
					result.insert_impl(item.key(), item.value(), item.hashv());

			return result;
		}

		self& operator-= (const self& other)
		{
			if (this == &other)
			{
				clear();
				return *this;
			}

			for (auto&& key : other)
				pop(key);

			return *this;
		}

		bool __equals__(const self& other) const
		{
			if (size() != other.size()) return false;
			for (auto&& key : keys())
				if (!other.contains(key) || get(key) != other.get(key))
					return false;

			return true;
		}

		CString __str__() const
		{
			std::stringstream stream;
			stream << "{";
			for (auto&& item : items())
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

		Iterator begin() { return keys_.begin(); }

		Iterator end() { return keys_.end(); }

		ConstIterator begin() const { return keys_.begin(); }

		ConstIterator end() const { return keys_.end(); }
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

		BiChain<Key_t> keys_;

		static constexpr double MAX_LOAD_FACTOR = 0.75;

		static constexpr c_size MIN_BUCKET_SIZE = 8;
	};
}
#endif