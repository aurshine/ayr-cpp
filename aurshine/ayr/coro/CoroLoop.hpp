#ifndef AYR_CORO_EVENTLOOP_HPP
#define AYR_CORO_EVENTLOOP_HPP

#include "Task.hpp"

namespace ayr
{
	namespace coro
	{
#define std_resume __builtin_coro_resume;

		// 恢复一个协程
		def resume(Coroutine coroutine) { coroutine.resume(); }

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

			template<typename P>
			static P& async_run(std::coroutine_handle<P> coroutine)
			{
				while (!coroutine.done())
					coroutine.resume();

				if constexpr (std::is_void_v<P>)
					return;
				else
					return coroutine.promise();
			}
		};
	}
}
#endif