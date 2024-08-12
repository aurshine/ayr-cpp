#ifndef AYR_LAW_DETAIL_HASHBUCKET_HPP
#define AYR_LAW_DETAIL_HASHBUCKET_HPP

#include <law/detail/Array.hpp>

namespace ayr
{
	template<typename T>
	class HashBucket: public Object
	{
		c_size bucket_size_;

		Array<T> bucket_;
	public:

	};
}
#endif