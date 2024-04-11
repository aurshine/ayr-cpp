#pragma once
#include <thread>
#include <condition_variable>
#include <queue>
#include <functional>
#include "law/law.hpp"

namespace ayr
{
	class ThreadPool : public Object
	{
	public:
		ThreadPool(size_t num_pool)
			:threads(num_pool), stop(false)
		{
			auto work = [this]() {
				while (!this->stop)
				{
					std::unique_lock<std::mutex> lock(this->mtx);
					this->condition.wait(lock, [this]() {
						return this->stop || this->tasks.size();
						});

					if (this->stop)	return;
					auto task = this->tasks.front();
					this->tasks.pop();

					lock.unlock();
					task();
				}
			};
			for (auto& t : this->threads)
				t = std::thread(work);
		}

		~ThreadPool()
		{
			std::unique_lock<std::mutex> lock(this->mtx);
			this->stop = true;
			lock.unlock();
			this->condition.notify_all();
			for (auto& t : this->threads)
				if (t.joinable())
					t.join();
		}

		ThreadPool(const ThreadPool& pool) = delete;

		ThreadPool& operator= (const ThreadPool& pool) = delete;

		template<class Func, class ...Args>
		void push(Func&& func, Args&& ...args)
		{
			std::unique_lock<std::mutex> lock(this->mtx);

			this->tasks.push(std::bind(func, std::forward<Args>(args)...));
			lock.unlock();
			condition.notify_one();
		}


	private:
		std::vector<std::thread> threads;

		std::queue<std::function<void()>> tasks;

		std::mutex mtx;

		std::condition_variable condition;

		bool stop;
	};
}