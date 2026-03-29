#ifndef AYR_BASE_SHARED_HPP
#define AYR_BASE_SHARED_HPP

#include <atomic>

#include "raise_error.hpp"

namespace ayr
{
    template<typename T>
    class _AtomicCounter
    {
        std::atomic<int> count_;
    public:
        T value;

        template<typename... Args>
        _AtomicCounter(Args&&... args) : value(std::forward<Args>(args)...), count_(1) {}

        int increment() { return count_.fetch_add(1, std::memory_order_relaxed) + 1; }

        int decrement() { return count_.fetch_sub(1, std::memory_order_acq_rel) - 1; }
    };

    /*
	* @brief 引用计数器，线程安全
    * 
	* @details 该类实现了一个线程安全的引用计数器，允许多个Shared对象共享同一个被管理的对象。当最后一个Shared对象被销毁时，被管理的对象也会被销毁。
    * 
	* Shared不是智能指针，不提供指针语义。
    * 
    * Shared属于引用管理资源的类
    * 
    * @tparam T 被管理的对象类型
    */
    template<typename T>
    class Shared
    {
        using self = Shared<T>;

        _AtomicCounter<T>* counter_;
    public:
        template<typename... Args>
        Shared(Args&&... args) : counter_(ayr_make<_AtomicCounter<T>>(std::forward<Args>(args)...)) {}

        Shared(self& other) : counter_(other.counter_) { increment(); }

        Shared(const self& other) : counter_(other.counter_) { increment(); }

        Shared(self&& other) : counter_(other.counter_) { other.counter_ = nullptr; }

        ~Shared() { release(); }

        self& operator=(const self& other)
        {
            if (this == &other || counter_ == other.counter_) return *this;
            release();
            counter_ = other.counter_;
            increment();
            return *this;
        }

        self& operator=(self&& other)
        {
            if (this == &other || counter_ == other.counter_) return *this;
            release();
            counter_ = other.counter_;
            other.counter_ = nullptr;
            return *this;
        }

		// 提供解引用操作符，允许访问被管理的对象
        T& operator*() const
        {
            if (counter_ == nullptr)
                RuntimeError("Dereferencing a null Shared Resource");
            return counter_->value; 
        }

		// 提供指针语义访问资源
		T* operator->() const
        {
            if (counter_ == nullptr)
                RuntimeError("Dereferencing a null Shared Resource");
            return &counter_->value; 
        }

		// 释放资源
        void release()
        {
            if (decrement() == 0)
                ayr_desloc(counter_);
            counter_ = nullptr;
        }
    private:
		// 增加引用计数
        int increment()
        {
            if (counter_)
                return counter_->increment();
            return -1;
        }

		// 减少引用计数
        int decrement()
        {
            if (counter_)
                return counter_->decrement();
            return -1;
        }
    };
}
#endif // AYR_BASE_SHARED_HPP