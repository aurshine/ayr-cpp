#ifndef AYR_CORO_WHEN_HPP
#define AYR_CORO_WHEN_HPP

#include <tuple>

#include "CoroLoop.hpp"

namespace ayr
{
	namespace coro
	{
		template<typename... Ts>
		std::tuple<Ts...> when_all(Ts&&... coroutines)
		{
			static_assert(sizeof...(Ts) > 0, "No coroutines provided to when");

		}
	}
}
#endif