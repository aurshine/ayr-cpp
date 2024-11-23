#ifndef AYR_WRAPPER_HPP
#define AYR_WRAPPER_HPP

#include <type_traits>
#include <functional>

#include <ayr/detail/Object.hpp>


namespace ayr
{
	struct ScopeFunc : Object<ScopeFunc>
	{
		ScopeFunc(std::function<void()> f) : f_(f) {}

		~ScopeFunc() { f_(); }

		std::function<void()> f_;
	};

	template<typename Derived>
	struct Wrapper : public Object<Wrapper<Derived>>
	{
	public:
		template<typename F, typename ...Args>
		auto operator()(F&& call_, Args&&... args)
		{
			Derived* derived = static_cast<Derived*>(this);
			derived->into();
			ScopeFunc stop([derived]() { derived->escape(); });
			return call_(std::forward<Args>(args)...);
		}
	};
}
#endif