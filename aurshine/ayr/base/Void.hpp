#ifndef AYR_BASE_VOID_HPP
#define AYR_BASE_VOID_HPP

#include "Object.hpp"

namespace ayr
{
	struct Void : public Object<Void>
	{
		constexpr Void() = default;

		constexpr Void(const Void&) = default;

		constexpr Void& operator=(const Void&) { return *this; };

		~Void() = default;
	};

	template<typename T>
	using Voo = std::conditional_t<std::is_void_v<T>, Void, T>;
}
#endif