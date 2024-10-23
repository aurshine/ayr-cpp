#ifndef AYR_CORO_EVENTLOOP_HPP
#define AYR_CORO_EVENTLOOP_HPP

#include "Task.hpp"

namespace ayr
{
	namespace coro
	{
		class CoroLoop : Object<CoroLoop>
		{
			using self = CoroLoop;

			CoroLoop() {}
		public:
			CoroLoop(const self&) = delete;

			CoroLoop(self&&) = delete;

			self& operator=(const self&) = delete;

			self& operator=(self&&) = delete;

			static self& coroloop()
			{
				static self coro_loop;
				return coro_loop;
			}
		};
	}
}
#endif