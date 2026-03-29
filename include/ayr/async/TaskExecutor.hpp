#ifndef AYR_ASYNC_TASK_EXECUTOR_HPP
#define AYR_ASYNC_TASK_EXECUTOR_HPP

#include "../air/DynArray.hpp"
#include "./ThreadPool.hpp"

namespace ayr
{
	namespace async
	{
		class AsyncTask
		{
			using self = AsyncTask;

			friend class AsyncExecutor;

			// 任务的前置任务数量
			std::atomic<int> pre_;

			// 任务所属的执行器
			AsyncExecutor* executor_;

			DynArray<Shared<self>> childs_;
		public:
			using Task = std::function<void()>;

			AsyncTask(AsyncExecutor* executor) : pre_(0), executor_(executor), childs_(), task_() {}

			AsyncTask(AsyncExecutor* executor, Task t) : pre_(0), executor_(executor), childs_(), task_(t) {}

			AsyncTask(self&& other) noexcept : pre_(other.pre_.load()), executor_(other.executor_), childs_(std::move(other.childs_)), task_(std::move(other.task_)) {}

			self& operator=(self&& other) noexcept
			{
				if (this == &other) return *this;
				pre_ = other.pre_.load();
				executor_ = other.executor_;
				childs_ = std::move(other.childs_);
				task_ = std::move(other.task_);
				return *this;
			}

			/*
			* @brief 添加一个子任务到当前任务
			*
			* @param child 要添加的子任务
			*
			* @throws RuntimeError 如果子任务不属于同一个执行器
			*/
			void then(Shared<self> child)
			{
				//if (child == this) return; // 防止添加自己为子任务
				if (child->executor_ != executor_)
					RuntimeError("Child task must belong to the same executor");
				++child->pre_;
				childs_.append(child);
			}
		private:
			void action(ThreadPool& pool)
			{
				if (task_) task_();
				for (auto&& child : childs_)
					if (--child->pre_ == 0)
						pool.push([child, &pool]() mutable { child->action(pool); });
			}

			Task task_;
		};

		class AsyncExecutor
		{
			using self = AsyncExecutor;

			Shared<AsyncTask> root;

			ThreadPool pool_;
		public:
			AsyncExecutor(c_size num_work = 1) : root(this), pool_(num_work) {}

			/*
			* @brief 创建一个新的异步任务，并将其添加到执行器中
			*
			* @param task 要执行的任务
			*
			* @return Shared<AsyncTask> 返回创建的任务的共享指针
			*/
			Shared<AsyncTask> create_task(std::function<void()> task)
			{
				Shared<AsyncTask> t(this, std::move(task));
<<<<<<< HEAD
				root->then(t);
=======
				root->add_child(t);
>>>>>>> c97719c ([ADD]新增异步任务执行框架，使用拓扑排序支持任务依赖关系梳理)
				return t;
			}

			template<AnyRCallable F>
			std::pair<Shared<AsyncTask>, std::future<std::invoke_result_t<F>>> create_ret_task(F&& task)
			{
				using R = std::invoke_result_t<F>;
				Shared<std::packaged_task<R()>> f(std::forward<F>(task));
				Shared<AsyncTask> t(this, [f] { (*f)(); });
<<<<<<< HEAD
				root->then(t);
=======
				root->add_child(t);
>>>>>>> c97719c ([ADD]新增异步任务执行框架，使用拓扑排序支持任务依赖关系梳理)
				return { t, f->get_future() };
			}

			// 执行所有任务
			void run()
			{
				pool_.push([&] { this->root->action(pool_); });
				pool_.wait();
			}

			// 清空所有任务
<<<<<<< HEAD
			void clear() { root->childs_.clear(); }
=======
			void clear()
			{
				root->childs_.clear();
			}
>>>>>>> c97719c ([ADD]新增异步任务执行框架，使用拓扑排序支持任务依赖关系梳理)
		};
	}
}
#endif // AYR_ASYNC_TASK_EXECUTOR_HPP