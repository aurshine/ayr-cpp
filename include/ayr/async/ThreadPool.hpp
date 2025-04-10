#ifndef AYR_ASYNC_THREADPOOL_HPP
#define AYR_ASYNC_THREADPOOL_HPP

#include <thread>
#include <queue>
#include <future>
#include <functional>
#include <condition_variable>

#include "../Array.hpp"


namespace ayr
{
	namespace async
	{
		struct StopFlag
		{
			// 不停止
			static constexpr int8_t WAIT_FOREVER = -1;

			// 立即停止
			static constexpr int8_t STOP_NOW = 0;

			// 等待所有任务完成后停止
			static constexpr int8_t STOP_FINISHED = 1;
		};


		/*线程池类, 当线程池对象被销毁时, 任务队列的里的任务将会被清空*/
		class ThreadPool : public Object<ThreadPool>
		{
			using self = ThreadPool;

		public:
			using PoolTask = std::function<void()>;

			// 线程池构造函数，传入线程池的线程数量
			ThreadPool(size_t num_pool) :threads(num_pool), stop_flag(StopFlag::WAIT_FOREVER)
			{
				auto work = [this]()
					{
						while (true)
						{
							std::unique_lock<std::mutex> lock(this->mtx);
							this->condition.wait(lock, [this]() {
								return this->check_stop_now() || this->check_task_finished() || this->check_has_task();
								});

							if (this->check_stop_now() || this->check_task_finished())	return;
							PoolTask task = std::move(this->tasks.front());
							this->tasks.pop();

							lock.unlock();
							task();
						}
					};
				for (auto& t : this->threads)
					t = std::thread(work);
			}

			~ThreadPool() { stop(); }

			template<class F, class ...Args>
			auto push(F&& task, Args&& ...args) -> std::future<std::invoke_result_t<F, Args...>> {
				using ResultType = std::invoke_result_t<F, Args...>;

				auto pkg_task = std::make_shared<std::packaged_task<ResultType()>>(
					std::bind(std::forward<F>(task), std::forward<Args>(args)...)
				);

				std::future<ResultType> future = pkg_task->get_future();

				{
					std::lock_guard<std::mutex> lock(this->mtx);
					tasks.push([pkg_task]() { (*pkg_task)(); });
				}

				condition.notify_one();
				return future;
			}

			// 立即停止线程池
			void stop() { set_stop_flag(StopFlag::STOP_NOW); join(); }

			// 等待所有任务完成后停止线程池
			void wait() { set_stop_flag(StopFlag::STOP_FINISHED); join(); }
		private:
			void join()
			{
				for (auto& t : threads)
					if (t.joinable())
						t.join();
			}

			void set_stop_flag(int8_t flag)
			{
				{
					std::lock_guard<std::mutex> lock(mtx);
					stop_flag = flag;
				}
				condition.notify_all();
			}

			// 检查是否立即停止
			// 默认此时持有锁
			bool check_stop_now() const
			{
				return stop_flag == StopFlag::STOP_NOW;
			}

			// 检查任务是否完成
			bool check_task_finished() const
			{
				return stop_flag == StopFlag::STOP_FINISHED && tasks.empty();
			}

			bool check_has_task() const
			{
				return !tasks.empty();
			}

			Array<std::thread> threads;

			std::queue<PoolTask> tasks;

			std::mutex mtx;

			std::condition_variable condition;

			int8_t stop_flag;
		};
	}
}

#endif // AYR_THREAD_THREADPOOL_HPP