#pragma once
#include <functional>

#include <law/printer.hpp>


namespace ayr
{
	class Wrapper : public Object
	{
	public:
		virtual void start() {}

		virtual void stop() {}

		template<typename F, typename ...Args>
		auto operator()(F&& func, Args&&... args)
		{
			start();
			auto ret = func(std::forward<Args>(args)...);
			stop();

			return ret;
		}
	};


	class VoidWrapper : public Object
	{
	public:
		virtual void start() {}

		virtual void stop() {}

		template<typename F, typename ...Args>
		void operator()(F&& func, Args&&... args)
		{
			start();
			func(std::forward<Args>(args)...);
			stop();
		}
	};
}
