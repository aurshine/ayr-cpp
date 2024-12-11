#ifndef AYR_SER_HPP
#define AYR_SER_HPP

#include <algorithm>

#include "ayr_memory.hpp"
#include "Atring.hpp"
#include "base/bunit.hpp"
#include "base/HashBucket.hpp"


namespace ayr
{
	template<Hashable T, typename Bucket = RobinHashBucket<T>>
	class Set : public Object<Set<T, Bucket>>
	{
		using self = Set<T>;

		using Bucket_t = Bucket;

	public:
		using Value_t = T;

		using Iterator = Bucket_t::Iterator;

		using ConstIterator = Bucket_t::ConstIterator;

		Set(c_size size = MIN_BUCKET_SIZE) : size_(0), bucket_(size) {}

		Set(std::initializer_list<Value_t>&& il) : Set(roundup2(c_size(il.size() / MAX_LOAD_FACTOR)))
		{
			for (auto& v : il)
				insert(v);
		}

		Set(const Set& other) : size_(0), bucket_(other.bucket_) {}

		Set(Set&& other) noexcept : size_(other.size_), bucket_(std::move(other.bucket_)) { other.size_ = 0; }

		~Set() {}

		Set& operator=(const Set& other)
		{
			if (this == &other) return *this;
			return *ayr_construct(this, other);
		}

		Set& operator=(Set&& other) noexcept
		{
			if (this == &other) return *this;
			return *ayr_construct(this, std::move(other));
		}

		c_size size() const { return size_; }

		c_size capacity() const { return bucket_.capacity(); }

		bool contains(const Value_t& value) const { return contains_hashv(ayrhash(value)); }

		double load_factor() const { return 1.0 * size() / capacity(); }

		template<typename _V>
		void insert(_V&& value)
		{
			hash_t hashv = ayrhash(value);
			if (!contains_hashv(hashv))
				insert_impl(std::forward<_V>(value), hashv);
		}

		void pop(const Value_t& value)
		{
			bucket_.pop(ayrhash(value));
			--size_;
		}

		self operator& (const self& other) const
		{
			if (this == &other)
				return *this;
			else if (size() > other.size())
				return other & *this;

			self result((std::max)(capacity(), other.capacity()));
			for (auto&& v : *this)
				if (other.contains(v))
					result.insert_impl(v, ayrhash(v));

			return result;
		}

		self& operator&=(const self& other)
		{
			if (this == &other) return *this;
			*this = *this & other;
			return *this;
		}

		self operator| (const self& other) const
		{
			if (this == &other)
				return *this;
			else if (size() < other.size())
				return other | *this;

			self result(static_cast<c_size>((size() + other.size()) / MAX_LOAD_FACTOR));
			for (auto&& v : *this)
				result.insert_impl(v, ayrhash(v));

			for (auto&& v : other)
				result.insert(v);

			return result;
		}

		self& operator|=(const self& other)
		{
			if (this == &other) return *this;
			for (auto&& v : other)
				insert(v);
			return *this;
		}

		self operator^ (const self& other) const
		{
			if (this == &other)
				return self();

			self result(static_cast<c_size>(size() / MAX_LOAD_FACTOR));
			for (auto&& v : *this)
				if (!other.contains(v))
					result.insert_impl(v, ayrhash(v));

			for (auto&& v : other)
				if (!contains(v))
					result.insert_impl(v, ayrhash(v));

			return result;
		}

		self& operator^=(const self& other)
		{
			if (this == &other)
			{
				clear();
				return *this;
			}

			for (auto&& v : other)
				if (contains(v))
					pop(v);
				else
					insert_impl(v, ayrhash(v));

			return *this;
		}

		self operator+ (const self& other) const
		{
			return *this | other;
		}

		self& operator+=(const self& other)
		{
			return *this |= other;
		}

		self operator- (const self& other) const
		{
			if (this == &other)
				return self();

			self result(capacity());
			for (auto&& v : *this)
				if (!other.contains(v))
					result.insert_impl(v, ayrhash(v));

			return result;
		}

		self& operator-=(const self& other)
		{
			if (this == &other)
			{
				clear();
				return *this;
			}

			for (auto&& v : other)
				pop(v);

			return *this;
		}

		void clear() { bucket_.clear(); size_ = 0; }

		bool __equals__(const self& other)
		{
			if (size() != other.size())
				return false;

			for (auto v : *this)
				if (!other.contains(v))
					return false;

			return true;
		}

		CString __str__() const
		{
			std::stringstream stream;
			stream << "{";
			for (auto v : *this)
				stream << v << ", ";

			std::string str = stream.str();
			if (str.size() > 2)
			{
				str.pop_back();
				str.pop_back();
			}
			str.push_back('}');

			return str;
		}
	private:
		bool contains_hashv(hash_t hashv) const { return bucket_.contains(hashv); }

		void expand() { bucket_.expand(std::max<c_size>(capacity() * 2, MIN_BUCKET_SIZE)); }

		template<typename _V>
		void insert_impl(_V&& value) { insert_impl(std::forward<_V>(value), ayrhash(static_cast<const Value_t&>(value))); }

		template<typename _V>
		void insert_impl(_V&& value, hash_t hashv)
		{
			if (load_factor() >= MAX_LOAD_FACTOR)
				expand();

			bucket_.set_value(std::forward<_V>(value), hashv);
			++size_;
		}

		Value_t* get_impl(hash_t hashv) { return bucket_.try_get(hashv); }

		const Value_t* get_impl(hash_t hashv) const { return bucket_.try_get(hashv); }

		Iterator begin() { return bucket_.begin(); }

		Iterator end() { return bucket_.end(); }

		ConstIterator begin() const { return bucket_.begin(); }

		ConstIterator end() const { return bucket_.end(); }
	private:
		Bucket_t bucket_;

		c_size size_;

		static constexpr double MAX_LOAD_FACTOR = 0.75;

		static constexpr c_size MIN_BUCKET_SIZE = 8;
	};
}
#endif