#ifndef AYR_LAW_DETAIL_HASHBUCKET_HPP
#define AYR_LAW_DETAIL_HASHBUCKET_HPP

#include <law/detail/Array.hpp>
#include <law/detail/bunit.hpp>


namespace ayr
{
	class BucketPolicy : public Object
	{
	public:
		static c_size next_bucket_size(c_size current_size) { return roundup2(current_size + 1); }

		
	};


	template<typename T, typename Policy = BucketPolicy>
	class HashBucket: public Object
	{
		Array<T> bucket_;
	public:
		HashBucket(c_size bucket_size) : bucket_(bucket_size) {}

		// 判断是否过载，过载返回true
		bool check_overload(c_size used_size) const { return usd_size * 4ull > bucket_.size() * 3ull; }

		void expend()
		{
			Array<T> bucket_new = Array<T>{ Policy::next_bucket_size(bucket_.size())};
		}

		c_size get_index_by_hash_value(const T& key) const
		{

		}
	};
}
#endif