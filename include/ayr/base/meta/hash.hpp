#ifndef AYR_BASE_HASH_HPP
#define AYR_BASE_HASH_HPP

#include <cstring>

#include "ayr_concepts.hpp"

namespace ayr
{
	 // 混合 64 位整数，使高位信息充分影响用于取桶的低位
	constexpr def integer_hash(hash_t value) noexcept -> hash_t
	{
		value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
		value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
		return value ^ (value >> 31);
	}

	/**
	 * @brief 从原始内存读取一个 32 位无符号小端整数。
	 * @param data 指向至少 4 个连续可读字节，允许未对齐。
	 * @note 不推进调用方的指针，也不检查缓冲区长度。
	 */
	def load_u32_le(const void* data) noexcept -> uint32_t
	{
		const auto* p = static_cast<const uint8_t*>(data);

		return uint32_t(p[0])
			| uint32_t(p[1]) << 8
			| uint32_t(p[2]) << 16
			| uint32_t(p[3]) << 24;
	}

	// 计算字节序列哈希，保留既有四字节尾部及种子对应的哈希结果
	def bytes_hash(const char* data, size_t n, uint32_t seed = 0xbc9f1d34) -> hash_t
	{
		constexpr hash_t m = 0xc6a4a793;
		constexpr hash_t r = 24;
		hash_t h = seed ^ (n * m);

		// 以剩余长度控制读取，避免短输入形成越过数组末端的指针。
		// 保留恰好四字节时的尾部路径，以兼容现有字符串哈希。
		while (n > 4)
		{
			hash_t w = load_u32_le(data);
			data += 4;
			n -= 4;
			h = (h + w) * m;
			h ^= (h >> 16);
		}

		uint32_t tail = 0;
		for (size_t i = 0; i < n; ++i)
			tail |= static_cast<uint32_t>(static_cast<uint8_t>(data[i])) << (i * 8);
		
		h += static_cast<hash_t>(static_cast<int32_t>(tail));
		h *= m;
		h ^= (h >> r);
		return h;
	}

	// 优先使用自定义哈希，为整型和枚举使用快速混合，其余类型保留标准哈希
	template<Hashable K>
	def ayr_hash_impl(const K& key) -> hash_t
	{
		using Key = std::decay_t<K>;
		if constexpr (AyrLikeHashable<K>)
			return key.__hash__();
		else if constexpr (std::is_integral_v<Key> && sizeof(Key) <= sizeof(hash_t))
			return integer_hash(static_cast<hash_t>(key));
		else if constexpr (std::is_enum_v<Key> && sizeof(Key) <= sizeof(hash_t))
			return integer_hash(static_cast<hash_t>(static_cast<std::underlying_type_t<Key>>(key)));
		else
			return std::hash<Key>{}(key);
	}

	// 计算键的哈希；整数和枚举的哈希值不再依赖标准库实现
	template<Hashable K>
	def ayrhash(const K& key) { return ayr_hash_impl(key); }

	// 按以空字符结尾的字符串内容计算哈希
	def ayrhash(const char* key) { return bytes_hash(key, std::strlen(key)); }

	// 按字符数组内容计算哈希，不包含末尾的空字符
	template<size_t N>
	def ayrhash(const char(&key)[N]) { return bytes_hash(key, N - 1); }
}
#endif // AYR_BASE_HASH_HPP
