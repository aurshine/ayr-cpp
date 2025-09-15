#ifndef AYR_NET_SELECTOR_IOEVENT_HPP
#define AYR_NET_SELECTOR_IOEVENT_HPP

#include "../../base/View.hpp"
#include "../../coro/co_utils.hpp"

namespace ayr
{
	namespace net
	{
		/*
		* @brief IoEvent类
		*
		* @details 事件类，保存注册的事件和发生的事件
		*
		* @tparam Data 每个fd额外记录的数据类型
		*
		* @tparam Flag 事件类型
		*/
		class IoEvent : public Object<IoEvent>
		{
		public:
			using Data = coro::Coroutine;

			using Flag = uint32_t;

			constexpr static Flag NONE = 0;

			constexpr static Flag READABLE = 1;

			constexpr static Flag WRITABLE = 2;

			constexpr static Flag ERRORABLE = 4;
		private:
			using self = IoEvent;

			using super = Object<IoEvent>;

			// 注册的事件和发生的事件
			Flag registered_flags_, happened_flags_;

		public:
			Data data;

			IoEvent() : registered_flags_(NONE), happened_flags_(NONE), data() {}

			IoEvent(const Flag& flags, const Data& data) : registered_flags_(flags), happened_flags_(NONE), data(data) {}

			IoEvent(const self& other) : IoEvent(other.registered_flags_, other.data) {}

			self& operator=(const self& other)
			{
				if (this == &other) return *this;
				registered_flags_ = other.registered_flags_;
				data = other.data;
				return *this;
			}

			// 返回注册的事件
			Flag registered_events() const { return registered_flags_; }

			// 设置发生的事件
			void set_events(const Flag& flag) { happened_flags_ = flag; }

			// 返回发生的事件
			Flag events() const { return happened_flags_; }
		};
	}
}
#endif // AYR_NET_SELECTOR_IOEVENT_HPP