#ifndef AYR_BASE_EXTASK_HPP
#define AYR_BASE_EXTASK_HPP

#include "raise_error.hpp"

namespace ayr
{
	/*
	* @brief 作用域结束时执行的任务类
	* 
	* @tparam F 可调用对象类型，必须满足Invocable概念，即无参数的可调用对象
	*/
	template <Invocable F>
	class ExTask
	{
		using self = ExTask;

		F task_;
	public:
		ExTask(F task) : task_(std::move(task)) {}

		ExTask(F&& task) : task_(std::move(task)) {}

		ExTask(const self& other) : task_(other.task_) {}

		ExTask(self&& other) noexcept : task_(std::move(other.task_)) {}

		~ExTask() { task_(); }

		self& operator=(const self& other)
		{
			if (this == &other) return *this;
			task_ = other.task_;
			return *this;
		}

		self& operator=(self&& other) noexcept
		{
			if (this == &other) return *this;
			task_ = std::move(other.task_);
			return *this;
		}
	};

	template<Invocable F>
	ExTask<F> make_extask(F&& fn) { return ExTask<F>(std::forward<F>(fn)); }

#define exitask(task) auto CONCAT(_ex_task_, __LINE__) = make_extask(task);
}
#endif // AYR_BASE_EXTASK_HPP