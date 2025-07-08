#ifndef AYR_BASE_EXTASK_HPP
#define AYR_BASE_EXTASK_HPP

#include <functional>

#include "raise_error.hpp"

namespace ayr
{
	/*
	* ExitTask 退出任务
	* 用于延迟执行退出操作，防止程序退出时发生未知错误
	* 调用 cancel() 取消任务
	* 调用 run() 执行任务
	* 调用 set() 设置任务
	*/
	class ExTask : public Object<ExTask>
	{
		using self = ExTask;

		using super = Object<ExTask>;

		using Task_t = std::function<void()>;

		Task_t task_;
	public:

		ExTask() {}

		ExTask(Task_t task) : task_(std::move(task)) {}

		ExTask(self&& other) noexcept : task_(std::move(other.task_)) {}

		~ExTask() { if (task_) task_(); }

		self& operator=(self&& other) noexcept
		{
			if (this == &other) return;
			task_ = std::move(other.task_);
			return *this;
		}

		// 取消任务
		void cancel() { task_ = nullptr; }

		// 提前执行任务
		void run()
		{
			if (task_) task_();
			cancel();
		}

		// 设置任务
		void set(Task_t task) { task_ = std::move(task); }
	};
}
#endif // AYR_BASE_EXTASK_HPP