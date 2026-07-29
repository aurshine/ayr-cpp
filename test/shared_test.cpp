#include <ayr/base.hpp>

#include <ayr/base/utest.hpp>

using namespace ayr;

namespace
{
    class Tracked
    {
    public:
        static inline int alive_count = 0;
        static inline int destroy_count = 0;

        int value;

        explicit Tracked(int value) : value(value)
        {
            ++alive_count;
        }

        ~Tracked()
        {
            --alive_count;
            ++destroy_count;
        }
    };
}

int main()
{
    // 空Shared不管理资源。
    Shared<int> empty;
    AYR_TEST_EXPECT(!empty);
    AYR_TEST_EXPECT(!empty.has_value());
    AYR_TEST_EXPECT_EQ(empty.use_count(), 0);
    AYR_TEST_EXPECT_AYR_ERROR(*empty);

    // 拷贝Shared只增加引用计数，并共享同一个对象。
    Shared<int> value(42);
    AYR_TEST_EXPECT(value);
    AYR_TEST_EXPECT_EQ(value.use_count(), 1);
    {
        Shared<int> copy = value;
        AYR_TEST_EXPECT_EQ(value.use_count(), 2);
        AYR_TEST_EXPECT_EQ(copy.use_count(), 2);
        *copy = 7;
        AYR_TEST_EXPECT_EQ(*value, 7);
    }
    AYR_TEST_EXPECT_EQ(value.use_count(), 1);

    // 移动Shared会清空源对象。
    Shared<int> moved = std::move(value);
    AYR_TEST_EXPECT(!value);
    AYR_TEST_EXPECT(moved);
    AYR_TEST_EXPECT_EQ(moved.use_count(), 1);
    AYR_TEST_EXPECT_EQ(*moved, 7);

    // 两个Shared指向同一控制块时，移动赋值仍需清空源对象。
    Shared<int> alias = moved;
    AYR_TEST_EXPECT_EQ(moved.use_count(), 2);
    moved = std::move(alias);
    AYR_TEST_EXPECT(!alias);
    AYR_TEST_EXPECT_EQ(moved.use_count(), 1);

    // 最后一个引用释放时，被管理对象只析构一次。
    Tracked::alive_count = 0;
    Tracked::destroy_count = 0;
    {
        Shared<Tracked> tracked(11);
        Shared<Tracked> tracked_copy = tracked;
        AYR_TEST_EXPECT_EQ(Tracked::alive_count, 1);
        AYR_TEST_EXPECT_EQ(tracked->value, 11);
        tracked_copy.reset();
        AYR_TEST_EXPECT(!tracked_copy);
        AYR_TEST_EXPECT_EQ(Tracked::alive_count, 1);
        AYR_TEST_EXPECT_EQ(tracked.use_count(), 1);
    }
    AYR_TEST_EXPECT_EQ(Tracked::alive_count, 0);
    AYR_TEST_EXPECT_EQ(Tracked::destroy_count, 1);
}
