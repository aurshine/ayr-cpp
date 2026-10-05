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
    UTEST_SCOPE("测试空 Shared 不管理资源。")
    {
        Shared<int> empty;
        UTEST_EXPECT(!empty);
        UTEST_EXPECT(!empty.has_value());
        UTEST_EXPECT_EQ(empty.use_count(), 0);
        UTEST_EXPECT_AYR_ERROR(*empty);
    };

    Shared<int> value(42);
    UTEST_SCOPE("测试拷贝 Shared 只增加引用计数，并共享同一个对象。")
    {
        UTEST_EXPECT(value);
        UTEST_EXPECT_EQ(value.use_count(), 1);
        {
            Shared<int> copy = value;
            UTEST_EXPECT_EQ(value.use_count(), 2);
            UTEST_EXPECT_EQ(copy.use_count(), 2);
            *copy = 7;
            UTEST_EXPECT_EQ(*value, 7);
        }
        UTEST_EXPECT_EQ(value.use_count(), 1);
    };

    Shared<int> moved;
    UTEST_SCOPE("测试移动 Shared 会清空源对象。")
    {
        moved = std::move(value);
        UTEST_EXPECT(!value);
        UTEST_EXPECT(moved);
        UTEST_EXPECT_EQ(moved.use_count(), 1);
        UTEST_EXPECT_EQ(*moved, 7);
    };

    UTEST_SCOPE("测试两个 Shared 指向同一控制块时移动赋值不会清空源对象。")
    {
        Shared<int> alias = moved;
        UTEST_EXPECT_EQ(moved.use_count(), 2);
        moved = std::move(alias);
        UTEST_EXPECT(alias);
        UTEST_EXPECT_EQ(moved.use_count(), 2);
    };

    UTEST_SCOPE("测试最后一个引用释放时被管理对象只析构一次。")
    {
        Tracked::alive_count = 0;
        Tracked::destroy_count = 0;
        {
            Shared<Tracked> tracked(11);
            Shared<Tracked> tracked_copy = tracked;
            UTEST_EXPECT_EQ(Tracked::alive_count, 1);
            UTEST_EXPECT_EQ(tracked->value, 11);
            tracked_copy = Shared<Tracked>();
            UTEST_EXPECT(!tracked_copy);
            UTEST_EXPECT_EQ(Tracked::alive_count, 1);
            UTEST_EXPECT_EQ(tracked.use_count(), 1);
        }
        UTEST_EXPECT_EQ(Tracked::alive_count, 0);
        UTEST_EXPECT_EQ(Tracked::destroy_count, 1);
    };

    return UTEST_COMPLETE();
}
