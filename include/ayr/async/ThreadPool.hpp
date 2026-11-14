#ifndef AYR_ASYNC_THREADPOOL_HPP
#define AYR_ASYNC_THREADPOOL_HPP

#include <thread>
#include <queue>
#include <future>
#include <condition_variable>

#include "../base.hpp"


namespace ayr
{
	namespace async
	{
		/*
		* @brief 线程池类，管理一组线程并执行提交的任务
		* 
		* 当线程池对象被销毁时，未完成的任务将被丢弃，正在执行的任务将继续执行直到完成。
		*/
		class ThreadPool
		{
			using self = ThreadPool;

			using PoolTask = std::function<void()>;

			// 停止标志，表示线程池的停止状态
			// bool stoped_;

			// 当前正在运行的任务数量
			std::atomic<int32_t> num_running_;

			Array<std::thread> threads;

			std::condition_variable condition;

			std::mutex mtx;

			std::queue<PoolTask> tasks;

			enum class PoolState
			{
				Running, // 正常运行中，接受新任务
				Draining, // 优雅关闭，拒绝新任务，等待所有任务完成后停止
				Stopping, // 尽快停止，拒绝新任务，丢弃所有未开始的任务
				Stopped // 已停止
			} state;
		public:
			// 线程池构造函数，传入线程池的线程数量
			ThreadPool(size_t num_pool) : 
				num_running_(0), 
				threads(num_pool),
				condition(),
				mtx(),
				state(PoolState::Running)
			{
				auto work = [this]()
					{
						while (true)
						{
							std::unique_lock<std::mutex> lock(this->mtx);
							this->condition.wait(lock, [this]() {
								// 当有等待的任务或者线程池不是运行状态，线程可以继续执行
								return this->has_waiting_task() || !this->running();
								});

							if (this->has_waiting_task())
							{
								if (this->running() || this->draining())
								{
									PoolTask task = std::move(this->tasks.front());
									this->tasks.pop();
									++this->num_running_;
									lock.unlock();
									
									task();
									--this->num_running_;
									condition.notify_all();
									continue;
								}
							}
							break;
						}
					};
				for (auto& t : this->threads)
					t = std::thread(work);
			}

			~ThreadPool() { if (!stoped()) stop(); }

			template<AnyRCallable F>
			std::future<std::invoke_result_t<F>> future_push(F&& task) 
			{
				if (!running())
					RuntimeError("Thread pool is not running, cannot push task");

				using R = std::invoke_result_t<F>;
				Shared<std::packaged_task<R()>> pkg_task(task);
				{
					std::lock_guard<std::mutex> lock(this->mtx);
					tasks.push([pkg_task] { (*pkg_task)(); });
				}
				condition.notify_one();
				return pkg_task->get_future();
			}

			template<AnyRCallable F>
			void push(F&& task)
			{
				if (!running())
					RuntimeError("Thread pool is not running, cannot push task");

				Shared<PoolTask> shared_task(std::forward<F>(task));
				{
					std::lock_guard<std::mutex> lock(this->mtx);
					tasks.push([shared_task] { (*shared_task)(); });
				}
				condition.notify_one();
			}

			// 运行线程池，等待所有任务完成
			// run完成后，线程池仍然可以接受新的任务，除非调用了 stop 或 wait 方法
			void run()
			{
				{
					std::unique_lock<std::mutex> lock(this->mtx);
					condition.wait(lock, [this]() { return !has_waiting_task() && !has_running_task(); });
				}
			}

			// 立即停止线程池，正在执行的任务将继续执行，丢弃所有未开始的任务
			// 调用 stop 后，线程池将不再接受新的任务
			void stop() 
			{
				{
					std::lock_guard<std::mutex> lock(this->mtx);
					state = PoolState::Stopping;
					condition.notify_all();
				}
				
				join(); 
				while (!tasks.empty()) tasks.pop();
			}

			// 等待线程池中的所有任务完成后停止线程池
			// 调用 wait 后，线程池将不再接受新的任务
			void wait() 
			{ 
				{
					std::lock_guard<std::mutex> lock(this->mtx);
					state = PoolState::Draining;
					condition.notify_all();
				}
	
				join(); 
			}
		private:
			// 等待所有线程完成
			void join()
			{
				for (auto& t : threads)
					if (t.joinable())
						t.join();
				state = PoolState::Stopped;
			}

			// 检查是否有任务正在等待执行
			bool has_waiting_task() const { return !tasks.empty(); }

			// 检查是否有任务正在执行
			bool has_running_task() const { return num_running_ > 0; }

			// 检查线程池是否正在运行
			bool running() const { return state == PoolState::Running; }

			// 检查线程池是否正在优雅关闭
			bool draining() const { return state == PoolState::Draining; }

			// 检查线程池是否正在停止
			bool stopping() const { return state == PoolState::Stopping; }

			// 检查线程池是否已经停止
			bool stoped() const { return state == PoolState::Stopped; }
		};
	}
}

#endif // AYR_THREAD_THREADPOOL_HPP