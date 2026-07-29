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

        // 增加引用计数
        void increment() { count_.fetch_add(1, std::memory_order_relaxed); }

        /**
        * @brief 减少引用计数
        *
        * @return bool 当前引用是否为最后一个引用
        */
        bool decrement() noexcept
        {
            if (count_.fetch_sub(1, std::memory_order_release) != 1)
                return false;

            std::atomic_thread_fence(std::memory_order_acquire);
            return true;
        }

        // 获取当前引用计数
        int use_count() const noexcept { return count_.load(std::memory_order_relaxed); }
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

        using ControlBlock = _AtomicCounter<T>;

        ControlBlock* counter_;
    public:
        // 创建一个不管理资源的空Shared
        constexpr Shared() noexcept : counter_(nullptr) {}

        template<typename Arg, typename... Args>
            requires (!std::same_as<std::remove_cvref_t<Arg>, self>) // 避免匹配到Shared(self& other)
        Shared(Arg&& arg, Args&&... args)
            : counter_(ayr_make<ControlBlock>(std::forward<Arg>(arg), std::forward<Args>(args)...)){}

        Shared(const self& other) : counter_(other.counter_) { increment(); }

        Shared(self&& other) : counter_(other.counter_) { other.counter_ = nullptr; }

        constexpr ~Shared() { if (counter_) { release(); } }

        self& operator=(const self& other)
        {
            if (counter_ == other.counter_) return *this;
            release();
            return *ayr_construct(this, other);
        }

        self& operator=(self&& other)
        {
            if (counter_ == other.counter_) return *this;
            release();
            return *ayr_construct(this, std::move(other));
        }

        // 判断当前是否管理对象
        constexpr bool has_value() const noexcept { return counter_ != nullptr; }

        // 判断当前是否管理对象
        constexpr operator bool() const noexcept { return has_value(); }

        // 获取当前共享引用数
        int use_count() const noexcept { return ifelse(counter_, counter_->use_count(), 0); }

        // 访问被管理对象
        T& operator*() const
        {
            if (!has_value())
                RuntimeError("Dereferencing a null Shared resource");
            return counter_->value;
        }

        // 访问被管理对象
        T* operator->() const
        {
            if (!has_value())
                RuntimeError("Dereferencing a null Shared resource");
            return &counter_->value;
        }

        // 释放当前引用并将Shared置空
        void release() noexcept
        {
            ControlBlock* counter = std::exchange(counter_, nullptr);
            if (counter && counter->decrement())
                ayr_desloc(counter);
        }
    private:
		// 增加引用计数
        void increment() { if (counter_) { counter_->increment(); } }
    };
}
#endif // AYR_BASE_SHARED_HPP