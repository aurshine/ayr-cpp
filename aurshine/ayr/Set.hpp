#ifndef AYR_SER_HPP
#define AYR_SER_HPP

#include <algorithm>
#include <ayr/ayr_memory.hpp>
#include <ayr/detail/bunit.hpp>
#include <ayr/detail/HashBucket.hpp>
#include <ayr/detail/RelationIterator.hpp>
#include <ayr/Atring.hpp>


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
				insert_impl(std::move(v), ayrhash(v));
		}

		Set(const Set& other) : bucket_(other.bucket_) {}

		Set(Set&& other) noexcept : bucket_(std::move(other.bucket_)) {}

		~Set() = default;

		Set& operator=(const Set& other)
		{
			if (this != &other)
				bucket_ = other.bucket_;
			return *this;
		}

		Set& operator=(Set&& other) noexcept
		{
			if (this != &other)
				bucket_ = std::move(other.bucket_);
			return *this;
		}

		c_size size() const { return size_; }

		c_size capacity() const { return bucket_.capacity(); }

		bool contains(const Value_t& value) const { return contains_hash(ayrhash(value)); }

		double load_factor() const { return 1.0 * size() / capacity(); }

		void insert(const Value_t& value)
		{
			hash_t hashv = ayrhash(value);
			if (!contains_hash(hashv))
				insert_impl(value, hashv);
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
		bool contains_hash(hash_t hashv) const { return bucket_.contains(hashv); }

		void expand() { bucket_.expand(std::max<c_size>(capacity() * 2, MIN_BUCKET_SIZE)); }

		void insert_impl(const Value_t& value, hash_t hashv)
		{
			if (load_factor() >= MAX_LOAD_FACTOR)
				expand();

			bucket_.set_store(value, hashv);
			++size_;
		}

		void insert_impl(Value_t&& value, hash_t hashv)
		{
			if (load_factor() >= MAX_LOAD_FACTOR)
				expand();

			bucket_.set_store(std::move(value), hashv);
			++size_;
		}


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