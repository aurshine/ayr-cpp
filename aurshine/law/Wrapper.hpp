#pragma once

#include <functional>

#include <law/printer.hpp>


namespace ayr
{
	class Wrapper : public Object
	{
	public:
		virtual void befor_function() {}

		virtual void after_function() {}

		template<typename F, typename ...Args>
		auto operator()(F&& func, Args&&... args)
		{
			befor_function();
			auto ret = func(std::forward<Args>(args)...);
			after_function();

			return ret;
		}
	};
}
