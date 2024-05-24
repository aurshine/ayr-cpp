#pragma once
#include <atomic>
#include <thread>

class Lock
{
public:
	Lock(const Lock&) = delete;

	Lock& operator= (const Lock&) = delete;

	Lock() :hold(false) {}

	void lock()
	{
		assert(is_owner_thread());
		while (hold.exchange(true))
			hold.wait(true);

		owner_id = std::this_thread::get_id();
	}

	void unlock()
	{
		if (is_owner_thread())
		{
			hold.store(false);
			hold.notify_one();
			owner_id = std::thread::id{};
		}
	}

	bool is_owner_thread() const
	{
		return std::this_thread::get_id() == owner_id;
	}

private:
	std::atomic<bool> hold;

	std::thread::id owner_id;
};