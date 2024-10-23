#ifndef AYR_WRAPPER_HPP
#define AYR_WRAPPER_HPP

#include <type_traits>
#include <ayr/detail/object.hpp>


namespace ayr
{
	class Wrapper : public Object<Wrapper>
	{
	public:
		virtual void start() {}

		virtual void stop() {}

		template<typename F, typename ...Args>
			requires issame<std::invoke_result_t<F, Args...>, void>
		void operator()(F&& call_, Args&&... args)
		{
			start();
			call_(std::forward<Args>(args)...);
			stop();
		}

		template<typename F, typename ...Args>
			requires (!issame<std::invoke_result_t<F, Args...>, void>)
		auto operator()(F&& call_, Args&&... args)
		{
			start();
			auto result = call_(std::forward<Args>(args)...);
			stop();
			return result;
		}
	};
}
#endif