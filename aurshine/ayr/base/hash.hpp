#ifndef AYR_DETAIL_HASH_HPP
#define AYR_DETAIL_HASH_HPP

#include <ayr/base/ayr_concepts.hpp>
#include <ayr/base/ayr.h>


namespace ayr
{
	template<Hashable K>
	inline hash_t ayr_hash_impl(const K& key, std::true_type) { return key.__hash__(); }

	template<Hashable K>
	inline hash_t ayr_hash_impl(const K& key, std::false_type) { return std::hash<K>{}(key); }

	template<Hashable K>
	inline hash_t ayrhash(const K& key) { return ayr_hash_impl(key, std::bool_constant<AyrLikeHashable<K>>{}); }

	inline uint32_t decode_fixed32(const char* ptr)
	{
		const uint8_t* buffer = reinterpret_cast<const uint8_t*>(ptr);

		return (
			static_cast<uint32_t>(buffer[0]) |
			(static_cast<uint32_t>(buffer[1]) << 8) |
			(static_cast<uint32_t>(buffer[2]) << 16) |
			(static_cast<uint32_t>(buffer[3]) << 24)
			);
	}


	inline hash_t bytes_hash(const char* data, size_t n, uint32_t seed = 0xbc9f1d34)
	{
		constexpr hash_t m = 0xc6a4a793;
		constexpr hash_t r = 24;
		const char* end = data + n;
		hash_t h = seed ^ (n * m);

		while (data + 4 < end)
		{
			hash_t w = decode_fixed32(data);
			data += 4;
			h = (h + w) * m;
			h ^= (h >> 16);
		}

		int dis = end - data;
		for (int i = 0; i < dis; i++)
			h += static_cast<uint8_t>(data[i]) << (i * 8);

		h *= m;
		h ^= (h >> r);
		return h;
	}
}
#endif