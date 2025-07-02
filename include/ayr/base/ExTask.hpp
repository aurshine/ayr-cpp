#ifndef AYR_BASE_EXTASK_HPP
#define AYR_BASE_EXTASK_HPP

#include <functional>

#include "raise_error.hpp"

namespace ayr
{
	class ExTask: public Object<ExTask>
	{
		using self = ExTask;

		using super = Object<ExTask>;

		using Task_t = std::function<void()>;

		Task_t task_;
	public:

		ExTask(Task_t&& task): task_(std::move(task)) {}

		~ExTask() { task_(); }
	};
}
#endif // AYR_BASE_EXTASK_HPP