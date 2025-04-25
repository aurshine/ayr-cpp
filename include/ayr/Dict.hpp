#ifndef AYR_DICT_HPP
#define AYR_DICT_HPP

#include <ranges>

#include "Array.hpp"
#include "Chain.hpp"
#include "base/Table.hpp"


namespace ayr
{
	template<Hashable K, typename V>
	class Dict : public Object<Dict<K, V>>
	{
		using self = Dict<K, V>;
	public:
		using Key_t = K;

		using Value_t = V;

		using KV_t = std::pair<const Key_t, Value_t>;

		using Table_t = Table<typename Chain<KV_t>::Node_t*>;

		using TableValue_t = typename Chain<KV_t>::Node_t*;

		struct _KeyIterator : public IteratorInfo<_KeyIterator, self, std::bidirectional_iterator_tag, const Key_t>
		{
			using ItInfo = IteratorInfo<_KeyIterator, self, std::bidirectional_iterator_tag, const Key_t>;

			using KV_IT = Chain<KV_t>::ConstIterator;

			KV_IT it;

			_KeyIterator() : it() {}

			_KeyIterator(const KV_IT& it) : it(it) {}

			_KeyIterator(const typename ItInfo::iterator_type& other) : it(other.it) {}

			typename ItInfo::iterator_type& operator=(const typename ItInfo::iterator_type& other)
			{
				it = other.it;
				return *this;
			}

			ItInfo::reference operator*() const { return it->first; }

			ItInfo::pointer operator->() const { return &it->first; }

			typename ItInfo::iterator_type& operator++()
			{
				++it;
				return *this;
			}

			typename ItInfo::iterator_type operator++(int)
			{
				auto ret = *this;
				++it;
				return ret;
			}

			typename ItInfo::iterator_type& operator--()
			{
				--it;
				return *this;
			}

			typename ItInfo::iterator_type operator--(int)
			{
				auto ret = *this;
				--it;
				return ret;
			}

			bool __equals__(const typename ItInfo::iterator_type& other) const { return it == other.it; }
		};

		template<bool IsConst>
		struct _ValueIterator : public IteratorInfo<_ValueIterator<IsConst>, self, std::bidirectional_iterator_tag, add_const_t<IsConst, Value_t>>
		{
			using ItInfo = IteratorInfo<_ValueIterator<IsConst>, self, std::bidirectional_iterator_tag, add_const_t<IsConst, Value_t>>;

			using KV_IT = std::conditional_t<IsConst, typename Chain<KV_t>::ConstIterator, typename Chain<KV_t>::Iterator>;

			KV_IT it;

			_ValueIterator() : it() {}

			_ValueIterator(const KV_IT& it) : it(it) {}

			_ValueIterator(const typename ItInfo::iterator_type& other) : it(other.it) {}

			typename ItInfo::iterator_type& operator=(const typename ItInfo::iterator_type& other)
			{
				it = other.it;
				return *this;
			}

			ItInfo::reference operator*() const { return it->second; }

			ItInfo::pointer operator->() const { return &it->second; }

			typename ItInfo::iterator_type& operator++()
			{
				++it;
				return *this;
			}

			typename ItInfo::iterator_type operator++(int)
			{
				auto ret = *this;
				++it;
				return ret;
			}

			typename ItInfo::iterator_type& operator--()
			{
				--it;
				return *this;
			}

			typename ItInfo::iterator_type operator--(int)
			{
				auto ret = *this;
				--it;
				return ret;
			}

			bool __equals__(const typename ItInfo::iterator_type& other) const { return it == other.it; }
		};

		using Iterator = _KeyIterator;
		using ConstIterator = _KeyIterator;
		using KeyIterator = _KeyIterator;
		using ConstKeyIterator = _KeyIterator;
		using ValueIterator = _ValueIterator<false>;
		using ConstValueIterator = _ValueIterator<true>;

		Dict() : htable_(), kv_chain_() {}

		Dict(c_size size) : htable_(size), kv_chain_() {}

		Dict(const std::initializer_list<KV_t>& il) : htable_(il.size()), kv_chain_()
		{
			for (auto& [key, value] : il)
				insert(key, value);
		}

		Dict(const self& other) : htable_(other.htable_), kv_chain_(other.kv_chain_) {}

		Dict(self&& other) noexcept : htable_(std::move(other.htable_)), kv_chain_(std::move(other.kv_chain_)) {}

		Dict& operator=(const self& other)
		{
			if (this == &other) return *this;
			ayr_destroy(this);
			return *ayr_construct(this, other);
		}

		Dict& operator=(self&& other) noexcept
		{
			if (this == &other) return *this;
			ayr_destroy(this);
			return *ayr_construct(this, std::move(other));
		}

		// key-value对的数量
		c_size size() const { return kv_chain_.size(); }

		// 桶容量
		c_size capacity() const { return htable_.capacity(); }

		bool contains(const Key_t& key) const { return htable_.contains(ayrhash(key)); }

		auto keys() const { return std::ranges::subrange(key_begin(), key_end()); }

		auto values() { return std::ranges::subrange(value_begin(), value_end()); }

		auto values() const { return std::ranges::subrange(value_begin(), value_end()); }

		auto items() { return std::ranges::subrange(kv_chain_.begin(), kv_chain_.end()); }

		auto items() const { return std::ranges::subrange(kv_chain_.begin(), kv_chain_.end()); }

		const Value_t& get(const Key_t& key) const
		{
			TableValue_t item = get_impl(ayrhash(key));
			if (item) return item->value.second;
			key_not_found_error(key);
			return None<Value_t>;
		}

		Value_t& get(const Key_t& key)
		{
			TableValue_t item = get_impl(ayrhash(key));
			if (item) return item->value.second;
			key_not_found_error(key);
			return None<Value_t>;
		}

		const Value_t& get(const Key_t& key, const Value_t& default_value) const
		{
			TableValue_t item = get_impl(ayrhash(key));
			if (item) return item->value.second;
			return default_value;
		}

		Value_t& get(const Key_t& key, Value_t& default_value)
		{
			TableValue_t item = get_impl(ayrhash(key));
			if (item) return item->value.second;
			return default_value;
		}

		const Value_t& operator[](const Key_t& key) const { return get(key); }

		Value_t& operator[](const Key_t& key)
		{
			auto [index, move_dist] = htable_.try_get(ayrhash(key));
			if (htable_.items_[index].used() && htable_.items_[index].hashv == ayrhash(key))
				return htable_.items_[index].value()->value.second;
			
			auto kv_node = kv_chain_.append(key, Value_t{});
			htable_.insert_value_on_index(index, ayrhash(key), move_dist, kv_node);
			return kv_node->value.second;
		}

		// 向字典中插入一个key-value对, 若key已经存在, 则覆盖原有值
		template<typename _K, typename _V>
		Value_t& insert(_K&& key, _V&& value)
		{
			return insert(std::forward<_K>(key),
				std::forward<_V>(value),
				ayrhash(key)
			);
		}

		template<typename _K, typename _V>
		Value_t& insert(_K&& key, _V&& value, hash_t hashv)
		{
			htable_.try_expand();
			auto [index, move_dist] = htable_.try_get(hashv);

			if (htable_.items_[index].used())
				if (htable_.items_[index].hashv == hashv)
					htable_.items_[index].value()->value.second = std::forward<_V>(value);
				else
				{
					htable_.insert_value_on_index(index, hashv, move_dist, kv_chain_.append(std::forward<_K>(key), std::forward<_V>(value)));
					++htable_.size_;
				}
			else
			{
				htable_.items_[index].set_empty_value(hashv, move_dist, kv_chain_.append(std::forward<_K>(key), std::forward<_V>(value)));
				++htable_.size_;
			}

			return htable_.items_[index].value()->value.second;
		}

		void setdefault(const Key_t& key, const Value_t& default_value)
		{
			hash_t hashv = ayrhash(key);
			auto [index, move_dist] = htable_.try_get(hashv);
			if (!htable_.items_[index].used() || htable_.items_[index].hashv != hashv)
				htable_.insert_value_on_index(index, hashv, move_dist, kv_chain_.append(key, default_value));
		}

		void pop(const Key_t& key)
		{
			hash_t hashv = ayrhash(key);
			auto [index, move_dist] = htable_.try_get(hashv);
			if (htable_.items_[index].used() && htable_.items_[index].hashv == hashv)
			{
				kv_chain_.pop(htable_.items_[index].value());
				htable_.pop_value_on_index(index, hashv, move_dist);
				return;
			}

			key_not_found_error(key);
		}

		void clear()
		{
			htable_.clear();
			kv_chain_.clear();
		}

		// 根据key都存在，取this的value
		self operator&(const self& other) const
		{
			if (this == &other) return *this;
			self result(std::min(size(), other.size()));
			for (auto& [key, value] : items())
				if (other.contains(key))
					result.insert(key, value);

			return result;
		}

		self& operator&=(const self& other) { return *this = *this & other; }

		// 根据key有一方存在，都存在的取this的value
		self operator|(const self& other) const
		{
			if (this == &other) return *this;
			self result(*this);
			for (auto& [key, value] : other.items())
				if (!result.contains(key))
					result.insert(key, value);

			return result;
		}

		// 根据key有一方存在，都存在的取this的value
		self operator|(self&& other) const
		{
			if (this == &other) return *this;
			self result(*this);
			for (auto& [key, value] : other.items())
				if (!result.contains(key))
					result.insert(std::move(key), std::move(value));
			return result;
		}

		self& operator|=(const self& other) { return *this = *this | other; }

		self& operator|=(self&& other) { return *this = *this | std::move(other); }

		self operator^(const self& other) const
		{
			if (this == &other) return self();
			self result(size() + other.size());
			for (auto& [key, value] : items())
				if (!other.contains(key))
					result.insert(key, value);

			for (auto& [key, value] : other.items())
				if (!contains(key))
					result.insert(key, value);
			return result;
		}

		self operator^(self&& other) const
		{
			if (this == &other) return self();
			self result(size() + other.size());
			for (auto& [key, value] : items())
				if (!other.contains(key))
					result.insert(key, value);

			for (auto& [key, value] : other.items())
				if (!contains(key))
					result.insert(std::move(key), std::move(value));
			return result;
		}

		self& operator^= (const self& other) { return *this = *this ^ other; }

		self& operator^= (self&& other) { return *this = *this ^ std::move(other); }

		CString __str__() const
		{
			DynArray<CString> strs;
			strs.append("{");
			for (auto& [key, value] : items())
			{
				strs.append(cstr(key));
				strs.append(": ");
				strs.append(cstr(value));
				strs.append(", ");
			}
			if (size() > 0) strs.pop_back();

			strs.append("}");
			return CString::cjoin(strs);
		}

		void __repr__(Buffer& buffer) const
		{
			buffer << "{";
			bool first = true;
			for (auto& [key, value] : items())
			{
				if (first)
					first = false;
				else
					buffer << ", ";

				buffer << cstr(key);
				buffer << ": ";
				buffer << cstr(value);
			}
			buffer << "}";
		}

		bool __equals__(const self& other) const
		{
			if (this == &other) return true;
			if (size() != other.size()) return false;
			for (auto& [key, value] : items())
				if (!other.contains(key) || other.get(key) != value)
					return false;
			return true;
		}

		void __swap__(self& other) noexcept
		{
			ayr::swap(htable_, other.htable_);
			ayr::swap(kv_chain_, other.kv_chain_);
		}

		Iterator begin() { return Iterator(kv_chain_.begin()); }

		Iterator end() { return Iterator(kv_chain_.end()); }

		ConstIterator begin() const { return ConstIterator(kv_chain_.begin()); }

		ConstIterator end() const { return ConstIterator(kv_chain_.end()); }

		ConstKeyIterator key_begin() const { return ConstKeyIterator(kv_chain_.begin()); }

		ConstKeyIterator key_end() const { return ConstKeyIterator(kv_chain_.end()); }

		ValueIterator value_begin() { return ValueIterator(kv_chain_.begin()); }

		ValueIterator value_end() { return ValueIterator(kv_chain_.end()); }

		ConstValueIterator value_begin() const { return ConstValueIterator(kv_chain_.begin()); }

		ConstValueIterator value_end() const { return ConstValueIterator(kv_chain_.end()); }
	private:
		TableValue_t get_impl(hash_t hashv) const
		{
			auto [index, move_dist] = htable_.try_get(hashv);

			if (htable_.items_[index].used() && htable_.items_[index].hashv == hashv)
				return htable_.items_[index].value();
			else
				return nullptr;
		}

		void key_not_found_error(const Key_t& key) const { RuntimeError(std::format("Key '{}' not found in dictionary.", key)); }

		Table_t htable_;

		Chain<KV_t> kv_chain_;
	};
}
#endif