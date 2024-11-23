#ifndef AYR_ASYNC_LOCK_HPP
#define AYR_ASYNC_LOCK_HPP

#include <atomic>
#include <thread>

namespace ayr
{
	class Lock
	{
	public:
		Lock(const Lock&) = delete;

		Lock& operator= (const Lock&) = delete;

		Lock() : hold(false) {}

		void lock()
		{
			bool expected = false;

			// 当hold和expected相等时才会将true写入hold
			// 当compare_exchange_strong返回false时，说明获得当前锁
			while (hold.compare_exchange_strong(expected, true))
			{
				expected = false;
				hold.wait(true);
			}
		}

		void unlock()
		{
			hold.store(false);
			hold.notify_one();
		}

		bool try_lock()
		{
			bool expected = false;
			return hold.compare_exchange_strong(expected, true);
		}
	private:
		// 锁是否被持有， true表示被持有，false表示未被持有
		std::atomic<bool> hold;
	};
}

#endif // AYR_ASYNC_LOCK_HPP